//
//  JBModalSettingsViewController.h
//  JBChartViewDemo
//
//  Created by Danny Ricciotti on 6/20/14.
//  Copyright (c) 2014 Jawbone. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface JBModalSettingsViewController : UIViewController

- (IBAction)textChanged:(UITextField *)sender;
@property (nonatomic) IBOutlet UITextField *textfield;

- (IBAction)saveButtonPressed:(UIButton *)sender;
- (IBAction)dismissButtonPressed:(UIButton *)sender;

@property (nonatomic) NSString *text;

@end
