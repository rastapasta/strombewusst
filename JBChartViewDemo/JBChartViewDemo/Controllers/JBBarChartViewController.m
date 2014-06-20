//
//  JBBarChartViewController.m
//  JBChartViewDemo
//
//  Created by Terry Worona on 11/5/13.
//  Copyright (c) 2013 Jawbone. All rights reserved.
//

#import "JBBarChartViewController.h"

// Views
#import "JBBarChartView.h"
#import "JBChartHeaderView.h"
#import "JBBarChartFooterView.h"
#import "JBChartInformationView.h"

// Model
#import "JBServer.h"

// Modal Controller
#import "JBModalSettingsViewController.h"

// Numerics
CGFloat const kJBBarChartViewControllerChartHeight = 250.0f;
CGFloat const kJBBarChartViewControllerChartPadding = 10.0f;
CGFloat const kJBBarChartViewControllerChartHeaderHeight = 80.0f;
CGFloat const kJBBarChartViewControllerChartHeaderPadding = 10.0f;
CGFloat const kJBBarChartViewControllerChartFooterHeight = 25.0f;
CGFloat const kJBBarChartViewControllerChartFooterPadding = 5.0f;
NSUInteger kJBBarChartViewControllerBarPadding = 1;
NSInteger const kJBBarChartViewControllerNumBars = 24;
NSInteger const kJBBarChartViewControllerMaxBarHeight = 10;
NSInteger const kJBBarChartViewControllerMinBarHeight = 5;

// Strings
NSString * const kJBBarChartViewControllerNavButtonViewKey = @"view";

@interface JBBarChartViewController () <JBBarChartViewDelegate, JBBarChartViewDataSource>

@property (nonatomic, strong) JBBarChartView *barChartView;
@property (nonatomic, strong) JBChartInformationView *pastInformationView;
@property (nonatomic, strong) JBChartInformationView *currentInformationView;
@property (nonatomic, strong) NSArray *chartData;
@property (nonatomic, strong) NSArray *columnSymbols;

//@property (nonatomic) NSInteger currentHourNumber;

@property (nonatomic) NSTimer *refreshTimer;

// Buttons
- (void)rightBarButtonPressed:(id)sender;

// Data
- (void)initFakeData;

@end

@implementation JBBarChartViewController

#pragma mark - Alloc/Init

- (id)init
{
    self = [super init];
    if (self)
    {
        [self initFakeData];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self)
    {
        [self initFakeData];
    }
    return self;
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self)
    {
        [self initFakeData];
    }
    return self;
}

#pragma mark - Date

- (void)initFakeData
{
    NSMutableArray *mutableChartData = [NSMutableArray array];
    for (int i=0; i<kJBBarChartViewControllerNumBars; i++)
    {
        [mutableChartData addObject:@0];

    }
    _chartData = [NSArray arrayWithArray:mutableChartData];
    
    
    NSMutableArray *hours = [NSMutableArray new];
    for ( int i = 0; i < 24; i++ ) {
        [hours addObject:[NSString stringWithFormat:@"%d Uhr", i]];
    }
    _columnSymbols = hours;
}

#pragma mark - View Lifecycle

- (void)loadView
{
    [super loadView];
    
    self.view.backgroundColor = kJBColorBarChartControllerBackground;
    self.navigationItem.rightBarButtonItem = [self chartToggleButtonWithTarget:self action:@selector(rightBarButtonPressed:)];

    self.barChartView = [[JBBarChartView alloc] init];
    self.barChartView.frame = CGRectMake(kJBBarChartViewControllerChartPadding, kJBBarChartViewControllerChartPadding, self.view.bounds.size.width - (kJBBarChartViewControllerChartPadding * 2), kJBBarChartViewControllerChartHeight);
    self.barChartView.delegate = self;
    self.barChartView.dataSource = self;
    self.barChartView.headerPadding = kJBBarChartViewControllerChartHeaderPadding;
    self.barChartView.minimumValue = 0.0f;
    self.barChartView.backgroundColor = kJBColorBarChartBackground;
    
    JBChartHeaderView *headerView = [[JBChartHeaderView alloc] initWithFrame:CGRectMake(kJBBarChartViewControllerChartPadding, ceil(self.view.bounds.size.height * 0.5) - ceil(kJBBarChartViewControllerChartHeaderHeight * 0.5), self.view.bounds.size.width - (kJBBarChartViewControllerChartPadding * 2), kJBBarChartViewControllerChartHeaderHeight)];
//    headerView.titleLabel.text = @"Your House";
//    headerView.subtitleLabel.text = @"and energy";
    headerView.separatorColor = kJBColorBarChartHeaderSeparatorColor;
    self.barChartView.headerView = headerView;
    
    JBBarChartFooterView *footerView = [[JBBarChartFooterView alloc] initWithFrame:CGRectMake(kJBBarChartViewControllerChartPadding, ceil(self.view.bounds.size.height * 0.5) - ceil(kJBBarChartViewControllerChartFooterHeight * 0.5), self.view.bounds.size.width - (kJBBarChartViewControllerChartPadding * 2), kJBBarChartViewControllerChartFooterHeight)];
    footerView.padding = kJBBarChartViewControllerChartFooterPadding;
    footerView.leftLabel.text = [[self.columnSymbols firstObject] uppercaseString];
    footerView.leftLabel.textColor = [UIColor whiteColor];
    footerView.rightLabel.text = [[self.columnSymbols lastObject] uppercaseString];
    footerView.rightLabel.textColor = [UIColor whiteColor];
    self.barChartView.footerView = footerView;
    
    self.pastInformationView = [[JBChartInformationView alloc] initWithFrame:CGRectMake(self.view.bounds.origin.x, CGRectGetMaxY(self.barChartView.frame), self.view.bounds.size.width, self.view.bounds.size.height - CGRectGetMaxY(self.barChartView.frame) - CGRectGetMaxY(self.navigationController.navigationBar.frame))];
    [self.view addSubview:self.pastInformationView];
    
    self.currentInformationView = [[JBChartInformationView alloc] initWithFrame:CGRectMake(self.view.bounds.origin.x, CGRectGetMaxY(self.barChartView.frame), self.view.bounds.size.width, self.view.bounds.size.height - CGRectGetMaxY(self.barChartView.frame) - CGRectGetMaxY(self.navigationController.navigationBar.frame))];
    [self.view addSubview:self.currentInformationView];
    
    UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(wasTapped:)];
    [self.currentInformationView addGestureRecognizer:tap];


    [self.view addSubview:self.barChartView];
    [self.barChartView reloadData];
    
    
    // timer
    self.refreshTimer = [NSTimer scheduledTimerWithTimeInterval:15 target:self selector:@selector(refreshTimerFired:) userInfo:nil repeats:YES];
}

- (void)dealloc
{
    [self.refreshTimer invalidate];
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    [self.barChartView setState:JBChartViewStateExpanded];
    
    [self.currentInformationView setHidden:NO animated:YES];
    [self.currentInformationView setTitleText:@"Aktuelle Nutzung"];
//    [self.currentInformationView setValueText:@"18" unitText:@"kWh"];
    
    [self.pastInformationView setHidden:YES animated:YES];
    
    [self refreshServerData];
    
    if ( ! [[NSUserDefaults standardUserDefaults] objectForKey:@"jb-device"] ) {
        [self showModalSettings];
    }
}

- (void)refreshTimerFired:(id)sender
{
    [self refreshServerData];
}

- (void)wasTapped:(id)sender
{
    [self refreshServerData];
}

- (void)refreshServerData
{
    [[JBServer sharedServer] getHistoryWithCompletion:^(id responseObject, NSError *error) {
        if ( error ) {
            // todo
        } else {
            [self updateDisplayFromNewData];
        }
    }];
    
}

- (void)updateDisplayFromNewData
{
    self.chartData = [[JBServer sharedServer] history];
    [self.chartView reloadData];
    
    NSInteger current = [JBServer sharedServer].current;
    
    [self.currentInformationView setValueText:[NSString stringWithFormat:@"%d", current] unitText:@"Watt"];
    
    JBChartHeaderView *v = self.barChartView.headerView;
    v.titleLabel.text = [JBServer sharedServer].chartTitle;
    v.subtitleLabel.text = [JBServer sharedServer].chartSubtitle;

}


#pragma mark - JBBarChartViewDelegate

- (CGFloat)barChartView:(JBBarChartView *)barChartView heightForBarViewAtAtIndex:(NSUInteger)index
{
    if ( index >= self.chartData.count ) {
        return 0;
    }
    return [[self.chartData objectAtIndex:index] floatValue];
}

#pragma mark - JBBarChartViewDataSource

- (NSUInteger)numberOfBarsInBarChartView:(JBBarChartView *)barChartView
{
    return kJBBarChartViewControllerNumBars;
}

- (NSUInteger)barPaddingForBarChartView:(JBBarChartView *)barChartView
{
    return kJBBarChartViewControllerBarPadding;
}

- (UIColor *)barChartView:(JBBarChartView *)barChartView colorForBarViewAtIndex:(NSUInteger)index
{
    return (index % 2 == 0) ? kJBColorBarChartBarBlue : kJBColorBarChartBarGreen;
}

- (UIColor *)barSelectionColorForBarChartView:(JBBarChartView *)barChartView
{
    return [UIColor whiteColor];
}

- (void)barChartView:(JBBarChartView *)barChartView didSelectBarAtIndex:(NSUInteger)index touchPoint:(CGPoint)touchPoint
{
    NSLog(@"Didselect %d", index);
    NSNumber *valueNumber = [self.chartData objectAtIndex:index];
    [self.pastInformationView setValueText:[NSString stringWithFormat:@"%@", valueNumber] unitText:@"kWh"];
    [self.pastInformationView setTitleText:[self.columnSymbols objectAtIndex:index]];
    [self.pastInformationView setHidden:NO animated:YES];
    [self setTooltipVisible:YES animated:YES atTouchPoint:touchPoint];
    [self.tooltipView setText:[self.columnSymbols objectAtIndex:index]];
    
    [self.currentInformationView setHidden:YES animated:YES];
}

- (void)didUnselectBarChartView:(JBBarChartView *)barChartView
{
    
    [self.currentInformationView setHidden:NO animated:YES];
    
    [self.pastInformationView setHidden:YES animated:YES];
    [self setTooltipVisible:NO animated:YES];
}

#pragma mark - Buttons

- (void)rightBarButtonPressed:(id)sender
{
    UIView *buttonImageView = [self.navigationItem.rightBarButtonItem valueForKey:kJBBarChartViewControllerNavButtonViewKey];
    buttonImageView.userInteractionEnabled = NO;
    
    CGAffineTransform transform = self.barChartView.state == JBChartViewStateExpanded ? CGAffineTransformMakeRotation(M_PI) : CGAffineTransformMakeRotation(0);
    buttonImageView.transform = transform;
    
    [self.barChartView setState:self.barChartView.state == JBChartViewStateExpanded ? JBChartViewStateCollapsed : JBChartViewStateExpanded animated:YES callback:^{
        buttonImageView.userInteractionEnabled = YES;
    }];
    
    [self showModalSettings];
    
}

- (void)showModalSettings
{
    JBModalSettingsViewController *modalController = [[JBModalSettingsViewController alloc] initWithNibName:nil bundle:nil];
    [self presentViewController:modalController animated:YES completion:^{
        
    }];

}

#pragma mark - Overrides

- (JBChartView *)chartView
{
    return self.barChartView;
}

@end
