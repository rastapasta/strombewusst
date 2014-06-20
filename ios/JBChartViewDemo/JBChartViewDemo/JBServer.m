//
//  JBServer.m
//  JBChartViewDemo
//
//  Created by Danny Ricciotti on 6/19/14.
//  Copyright (c) 2014 Jawbone. All rights reserved.
//

#import "JBServer.h"

@implementation JBServer

- (void)getHistoryWithCompletion:(JBServerCompletionBlock) completion
{
    NSString *device = [[NSUserDefaults standardUserDefaults] stringForKey:@"jb-device"];
    if ( !device ) {
        NSLog(@"Phone does not have device set in NSUserDefaults");
        if ( completion ) {
            completion([NSError errorWithDomain:@"jb-error" code:420 userInfo:nil], nil);
        }
        return;
    }
    
    
    [self sendAction:@"data" method:@"GET" parameters:@{@"device":device} completion:^(NSDictionary *responseObject, NSError *error) {
        
        if ( responseObject && !error ) {
                        
            self.history = [responseObject objectForKey:@"history"];
            self.current = [[responseObject objectForKey:@"current"] integerValue];
            
                        self.chartTitle = [responseObject objectForKey:@"title"];
                        self.chartSubtitle = [responseObject objectForKey:@"subtitle"];
        }
        
        if ( completion ) {
            completion(responseObject, error);
        }
    }];
}

+ (instancetype)sharedServer
{
    static JBServer *server = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        server = [[JBServer alloc] init];
    });
    return server;
}

@end
