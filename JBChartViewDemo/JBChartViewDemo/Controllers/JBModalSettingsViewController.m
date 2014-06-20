//
//  JBModalSettingsViewController.m
//  JBChartViewDemo
//
//  Created by Danny Ricciotti on 6/20/14.
//  Copyright (c) 2014 Jawbone. All rights reserved.
//

#import "JBModalSettingsViewController.h"

@interface JBModalSettingsViewController ()

@end

@implementation JBModalSettingsViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
    
    self.textfield.text = [[NSUserDefaults standardUserDefaults] stringForKey:@"jb-device"];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    [self.textfield becomeFirstResponder];
}

- (IBAction)textChanged:(UITextField *)sender
{
    NSLog(@"new text=%@",sender.text);
}

- (IBAction)saveButtonPressed:(UIButton *)sender {
    
    
    NSString *newID = [self.textfield text];
    
    if ( newID.length < 1 ) {
        return;
    }
    NSLog(@"Saving %@", newID);
    
    [[NSUserDefaults standardUserDefaults] setObject:newID forKey:@"jb-device"];
    [[NSUserDefaults standardUserDefaults] synchronize];
    
    [self.presentingViewController dismissViewControllerAnimated:YES completion:^{
        
    }];
}

- (IBAction)dismissButtonPressed:(UIButton *)sender {
    
    NSString *device = [[NSUserDefaults standardUserDefaults] stringForKey:@"jb-device"];
    if ( !device ) {
        return;
    }
    
    [self.presentingViewController dismissViewControllerAnimated:YES completion:^{
        
    }];
}


@end
