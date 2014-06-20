//
//  EAServer.m
//  evilapples
//
//  Created by Danny on 1/4/13.
//  Copyright (c) 2013 Danny Ricciotti. All rights reserved.
//

////////////////////////////////////////////////////////////////////////////

// Class-specific constants

// Server URL dictated by directives in build.h

// TODO move to JBServer when used w/ mikes project

static const NSInteger EAServerPortUndefined = -1;
NSString * const EAServerProtocol = @"http://";
NSString * const EAServerAddress = @"se.esse.es";
NSInteger const EAServerPort = 8000;

// Allows us to add an artificial minimum delay to network requests. Useful for debugging in local enviroment and testing UI.
#ifdef DEBUG
static const NSTimeInterval kMinDelay = 1.0;
#else
static const NSTimeInterval kMinDelay = 0;
#endif

////////////////////////////////////////////////////////////////////////////

#import "EAServer.h"

#pragma mark -

@interface EAServer ()

@end

#pragma mark -

@implementation EAServer

#pragma mark - Public

- (id)init
{
    self = [super init];
    if (self) {
    }
    return self;
}

+ (instancetype)sharedServer
{
    static EAServer *server = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        server = [[EAServer alloc] init];
    });
    return server;
}

- (void)sendAction:(NSString *)action
            method:(NSString *)method
        parameters:(id)param
        completion:(void (^)(id responseObject, NSError* error))handler
{
    [self sendAction:action method:method parameters:param authenticated:NO completion:handler];
}

- (void)sendAction:(NSString *)action
            method:(NSString *)method
        parameters:(id)param
     authenticated:(BOOL)authenticated
        completion:(void (^)(id responseObject, NSError* error))handler
{
    NSParameterAssert(action);
    NSParameterAssert(handler);
    NSParameterAssert(method);
    
    
    // Build request
    NSURLRequest *request = [self _buildRequestForAction:action method:method parameters:param authenticated:authenticated];
    if( !request ) {
        // this could happen if the cookies don't exist for an authenticated request. Fail here and other logic will handle the need for user to re-login.
        NSLog(@"Failed to build request");
        if ( handler ) {
            handler(nil, [NSError errorWithDomain:@"hello" code:123 userInfo:nil]);
        }
        return;
    }
    NSLog(@"Request is %@", request);
    
    
    // Start background task
    UIBackgroundTaskIdentifier taskID =[[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:^{
        NSLog(@"Background HTTP request about to expire.");
    }];
    NSDate *startTime = [NSDate new];
    
    
    // Send request
    [NSURLConnection sendAsynchronousRequest:request queue:[NSOperationQueue mainQueue] completionHandler:^(NSURLResponse *response, NSData *data, NSError *error)
     {
         
         NSAssert([NSThread isMainThread] == YES, @"Should be main thread");
         NSInteger statusCode = 0;
         if ( response )
         {
             NSAssert([response isKindOfClass:[NSHTTPURLResponse class]], @"invalid class");
             statusCode = [(NSHTTPURLResponse *)response statusCode];
         }
         
         
         // we always parse the response object if it exists. even if there is an error
         id responseObject = nil;;
         if ( data ) {
             NSError *jsonError = nil;
             responseObject = [NSJSONSerialization JSONObjectWithData:data options:NSJSONReadingMutableContainers error:&jsonError];
             if ( jsonError )
             {
                 NSLog(@"Error parsing json in server response: %@", jsonError);
                 responseObject = nil;
                 
                 if ( error ) {
                     // in case of JSON decode error AND server error then we use the server error
                     NSLog(@"Multiple errors for network request. %@ and JSON decode error", error);
                 } else {
                     // we can't consider this successful if JSON parsing failed so use the JSON error to inform user that request was not successful.
                     error = jsonError;
                 }
             }
         } else {
             NSLog(@"Response has not object");
         }
         
         
         // check for a 403 error or an invalid credentials error
         [self _checkResponseForInvalidCredentials:(NSHTTPURLResponse *)response responseObject:responseObject];
         
         
         // verify that the request was successful and response code was 200 (HTTP success). If no error exists but not a 200 response then we create an error.
         if (( ! error ) && ( statusCode != 200 ))
         {
             // create error if none exists
             error = [NSError errorWithDomain:@"hello" code:124 userInfo:nil];
         }
         
         
         // finish it up
         // we send the response object in case of error because it may contain data the client can use. The existence of the error object is still what tells the user whether the request was successful or now.
         [self _completeWithHandler:handler error:error response:responseObject timeStart:startTime taskID:taskID];
         
     }];
}

#pragma mark - Private

- (void)_checkResponseForInvalidCredentials:(NSHTTPURLResponse *)response responseObject:(id)responseObject
{
//    if (!response) {
//        return;
//    }
//    NSCAssert([response isKindOfClass:[NSHTTPURLResponse class]], @"Invalid class.");
//    NSInteger statusCode = [response statusCode];
//    
//    // in the 400 level of status codes, we check the Titto error code for additional information
//    if ( (statusCode >= 400) && ( statusCode < 500)) {
//        
//        if (statusCode == 403)
//        {
//            NSNumber *errorCode = (NSNumber *)[responseObject objectForKey:@"code" ofClass:@"NSNumber" mustExist:false];
//        }
//    }
}

- (void)_completeWithHandler:(void (^)(id responseObject, NSError* error))handler error:(NSError *)error response:(id)response timeStart:(NSDate *)startTime taskID:(UIBackgroundTaskIdentifier)taskID
{
    NSTimeInterval timeElapsed = -[startTime timeIntervalSinceNow];    // make positive
    
    NSTimeInterval delayInSeconds = MAX(kMinDelay - timeElapsed, 0);    // 0 delay if time elapsed is > min delay
    
    dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delayInSeconds * NSEC_PER_SEC));
    dispatch_after(popTime, dispatch_get_main_queue(), ^(void){
        
        if ( handler ) {
            handler(response, error);
        }
        
        // end the background task
        [[UIApplication sharedApplication] endBackgroundTask:taskID];
    });
    
}

+ (NSURL *)_serverURLForAction:(NSString *)action
{
    return [NSURL URLWithString:[self _serverURLStringForAction:action]];
}

+ (NSString *)_serverString
{
    return [NSString stringWithFormat:@"%@%@",
            EAServerProtocol,
            EAServerAddress];
}

+ (NSString *)_serverURLStringForAction:(NSString *)action
{
    // method is similar to _serverBaseURL
    NSParameterAssert(action);
    if ( action ) {
        
        NSString *port = (EAServerPort!=EAServerPortUndefined)?[NSString stringWithFormat:@":%@",@(EAServerPort)]:@"";   // dont specify port if port is not specified

        return [NSString stringWithFormat:@"%@%@/%@",
                [self _serverString],
                port,
                action];
    }
    return nil;
}

- (NSURL *)_serverBaseURL
{
    // used for cookie manager
    return [NSURL URLWithString:[EAServer _serverString]];
}

- (NSURLRequest *)_buildRequestForAction:(NSString *)action method:(NSString *)method parameters:(id)param authenticated:(BOOL)authenticated
{
    // build request
    BOOL isGet = [method isEqualToString:@"GET"];
    NSString *urlString = [EAServer _serverURLStringForAction:action];
    NSURL *url;
    
    
    // REST practices say that for a GET request you do not want to use body data. Instead use URL encoding which will conform more standardly with GET HTTP requests.
    // see: http://stackoverflow.com/questions/978061/http-get-with-request-body
    // Add parameters to URL if GET
    if ( isGet && param ) {
        urlString = [self _encodeURLString:urlString withParameters:param];
    }
    
    url = [NSURL URLWithString:urlString];
    NSParameterAssert(url);
    
    NSLog(@"Endpoint URL is: %@", url);
    NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:url
                                                           cachePolicy:NSURLRequestReloadIgnoringCacheData
                                                       timeoutInterval:60.0f];
    [request setHTTPMethod:method];
    
    // Set body data if DELETE, PUT, POST
    if ( !isGet ) {
        
        NSData *data;
        if ( param )
        {
            NSError *error = nil;
            data = [NSJSONSerialization dataWithJSONObject:param options:0 error:&error];
            if ( error ) {
                NSLog(@"Failed to JSON encode object for send action %@", action);
                NSParameterAssert(nil); // breakpoints in debug mode
            }
        }
        [request setHTTPBody:data];
        [request setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];
    }
    
    // Send iOS client version
    [request setValue:[EAServer _clientVersionString] forHTTPHeaderField:@"Client-Version"];
    
    
    // Add basic auth
    NSString *basicly = @"Basic dGl0dG86KiY2dGUyM0tKWUtCR2VkODdieQ==";
    [request setValue:basicly forHTTPHeaderField:@"Authorization"];     // from titto
    
    return request;
}


- (NSString *)_encodeURLString:(NSString *)url withParameters:(NSDictionary *)dict
{
    // there's probably a built-in method for encoding parameters into a URL. Until that day... we have this.
    
    NSMutableString *urlCopy = [url mutableCopy];
    BOOL first = YES;
    NSString *seperator = @"?";
    
    for (NSString *key in [dict allKeys])
    {
        NSCAssert([key isKindOfClass:[NSString class]], @"Invalid class.");
        NSString *value = [dict objectForKey:key];
        
        NSString *key2 = [key stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
        NSString *value2;
        if ( [value isKindOfClass:[NSString class]]) {
            value2 = [value stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
        } else {
            value2 = value;
        }
        
        [urlCopy appendString:[NSString stringWithFormat:@"%@%@=%@", seperator, key2, value2]];
        if ( first ) {
            first = NO;
            seperator = @"&";
        }
    }
    
    return urlCopy;
}

+ (NSString *)_clientVersionString {
    return [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"];
}

@end
