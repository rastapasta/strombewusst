//
//  EAServer.h
//  evilapples
//
//  Created by Danny on 1/4/13.
//  Copyright (c) 2013 Danny Ricciotti. All rights reserved.
//

#import <Foundation/Foundation.h>

extern NSString * const TTServerAddress;
extern NSString * const TTServerProtocol;

@interface EAServer : NSObject

+ (instancetype)sharedServer;

- (void)sendAction:(NSString *)action
            method:(NSString *)method
        parameters:(id)param
        completion:(void (^)(id responseObject, NSError* error))handler;

- (void)sendAction:(NSString *)action
            method:(NSString *)method
        parameters:(id)param
     authenticated:(BOOL)authenticated
        completion:(void (^)(id responseObject, NSError* error))handler;

@end
