//
//  JBServer.h
//  JBChartViewDemo
//
//  Created by Danny Ricciotti on 6/19/14.
//  Copyright (c) 2014 Jawbone. All rights reserved.
//

#import "EAServer.h"

@interface JBServer : EAServer

typedef void (^JBServerCompletionBlock)(id responseObject, NSError *error);

- (void)getHistoryWithCompletion:(JBServerCompletionBlock) completion;

@property (nonatomic) NSArray *history;
@property (nonatomic) NSInteger current;
@property (nonatomic) NSString *chartTitle;
@property (nonatomic) NSString *chartSubtitle;

+ (instancetype)sharedServer;

@end
