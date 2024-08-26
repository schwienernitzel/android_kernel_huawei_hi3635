//modify the para from 170 to 132

#ifndef __EFFECT_OV5648_FOXCONN_CHM_H__
#define __EFFECT_OV5648_FOXCONN_CHM_H__

/*isp_reg_t*/
{
#ifndef OVISP_DEBUG_MODE
	/*global*/
	{0x65000,0x3f},//[0]AWBgainenable[1]BlackpixelcancelingenableDPC[2]Whitepixelcancelingenable[3]Rawnormalenable[4]Lensonlinecontrol[5]LensShadingEnable[6]Rawscaleenable[7]binningcorrentionenable
	{0X65002,0x9f},//offuv-denoise
	{0X65001,0X6e},//=6f,openrecursivedenoise;=6eoffrecursivedenoise
	{0x6502b,0x8a},//disableRGBHdns,enablejpegycbcrconvertionmatrix
	{0x65063,0x0c},//[2]rawdnsenable,default:0x08
	/*BLC*/
	{0x6500c,0x00},//blctarget
	{0x6500d,0x40},
	{0x6500e,0x00},//blctarget
	{0x6500f,0x40},
	{0x1dac4,0x00},//enable
	{0x1dac0,0x00},//captureblc
	{0x1dac1,0x46},
	{0x1dac2,0x00},
	{0x1dac3,0x46},
	{0x1c468,0x00},//previewblc
	{0x1c469,0x40},
	{0x1c46a,0x00},
	{0x1c46b,0x40},

	//awboffset
	{0x1dac5,0x01},//captureoffsetenable(defaultfalse)
	{0x1dac6,0x02},//previewoffsetinlowgain
	{0x1dac7,0x00},
	{0x1dac8,0x00},
	{0x1dac9,0x03},
	{0x1daca,0x02},//previewoffsetinhighgain
	{0x1dacb,0x00},
	{0x1dacc,0x00},
	{0x1dacd,0x03},
	{0x1dace,0xfd},//captureoffsetinlowgain
	{0x1dacf,0x00},
	{0x1dad0,0x00},
	{0x1dad1,0xfa},
	{0x1dad2,0xfd},//captureoffsetinhighgain
	{0x1dad3,0x00},
	{0x1dad4,0x00},
	{0x1dad5,0xfa},
	{0x1dad6,0xa0},//lowgainthreshold
	{0x1dad7,0xc0},//highgainthreshold
	/*AEC*/
	{0x1c594,0x01},//AdvancedAecEnable
	{0x1c596,0x42},//y-target-high
	{0x1c146,0x32},//y-target
	{0x1c14a,0x03},
	{0x1c14b,0x06},
	{0x1c14c,0x06},//aecfaststep//
	{0x1c14e,0x03},//slowstep//08
	{0x1c140,0x01},//banding
	{0x1c13e,0x02},//realgainmodeforOV8830
	{0x66401,0x00},//windowweight
	{0x66402,0x80},//StatWin_Left
	{0x66403,0x00},
	{0x66404,0x60},//StatWin_Top
	{0x66405,0x00},
	{0x66406,0x80},//StatWin_Right
	{0x66407,0x00},
	{0x66408,0x60},//StatWin_Bottom
	{0x66409,0x01},//definitonofthecenter3x3window
	{0x6640a,0x1f},//nWin_Left
	{0x6640d,0x00},
	{0x6640e,0xd7},//nWin_Top
	{0x66411,0x02},
	{0x66412,0xbf},//nWin_Width
	{0x66415,0x02},
	{0x66416,0x0f},//nWin_Height
	{0x6642e,0x01},//nWin_Weight_0weightpass
	{0x6642f,0x01},//nWin_Weight_1
	{0x66430,0x01},//nWin_Weight_2
	{0x66431,0x01},//nWin_Weight_3
	{0x66432,0x09},//nWin_Weight_4
	{0x66433,0x09},//nWin_Weight_5
	{0x66434,0x09},//nWin_Weight_6
	{0x66435,0x09},//nWin_Weight_7
	{0x66436,0x12},//nWin_Weight_8
	{0x66437,0x09},//nWin_Weight_9
	{0x66438,0x09},//nWin_Weight_10
	{0x66439,0x09},//nWin_Weight_11
	{0x6643a,0x09},//nWin_Weight_12
	{0x6644e,0x03},//nWin_Weight_Shift
	{0x6644f,0x04},//blacklevel
	{0x66450,0xf8},//saturatelevel
	{0x6645b,0x1a},//blackweight1
	{0x6645d,0x1a},//blackweight2
	{0x66460,0x04},//saturateper1
	{0x66464,0x0a},//saturateper2
	{0x66467,0x14},//saturateweight1
	{0x66469,0x14},//saturateweight2
	/*hanchen+*/
	{0x66302,0xd8},//refbin
	{0x66303,0x06},//PsPer00a
	{0x66304,0x10},//PsPer1
	/*hanchen+*/
	/*autolevel*/
	//rawstrechmode2
	{0x66f00,0x01},//autostretchenalbe
	{0x66f01,0x00},//minlowlevel[11:8]
	{0x66f02,0x40},//minlowlevel[7:0]
	{0x66f03,0x00},//maxlowlevel[11:8]
	{0x66f04,0xa0},//maxlowlevel[7:0]
	{0x66f05,0x0f},//minhighlevel[11:8]
	{0x66f06,0xff},//minhighlevel[7:0],highlevel=255,disablehighlevel
	{0x66f09,0x10},//psthres1
	{0x66f0a,0x10},//psthres2
	{0x66f0b,0x02},//minnum
	{0x66f0c,0x00},
	{0x66f0d,0x00},//target_offset
	/*gamma*/
	{0x1d984,0x00},//adaptivegamma,disablefortestbyRichard@07092013
	{0x1c49b,0x01},//ManualGammaenable//
	{0x1c49c,0x01},//ManualGamma[15:8],gammavalue=currentvalue/256
	{0x1c49d,0x01},//ManualGamma[7:0]
	{0x1c49e,0x01},//RGBGamma[15:8]
	{0x1c49f,0x01},//RGBGamma[7:0]ManualgammaandRGBgammashouldbesame
	{0x1c4a2,0x00},//MaxRGBGammaGain[15:8]//shouldbecheckedbyRichard
	{0x1c4a3,0x40},//MaxRGBGammaGain[7:0]//shouldbecheckedbyRichard
	//thesesettingsshouldberemovedwhenadaptivegammaisdisabled
	{0x1d994,0x01},//lowgaingamma
	{0x1d995,0xe6},
	{0x1d996,0x01},//highgaingamma
	{0x1d997,0xc0},
	{0x1d998,0x01},//darkimagegamma
	{0x1d999,0xb3},
	{0x1d99a,0x88},//lowgainslope
	{0x1d99b,0x50},//highgainslope
	{0x1d99c,0x38},//darkimageslope
	{0x1d99d,0x14},//lowbrightthres
	{0x1d99e,0x20},//highbrightthres
	//curve
	{0x1c591,0x00},//enableadaptivecurve
	{0x1c592,0x40},//lowgainthreshold
	{0x1c593,0x70},//highgainthreshold
	/*ToneMappingcurve*/
	{0x6502f,0x00},//seperategammaandtonemap
	{0x65a00,0x1a},//bit[4]RGBorYUVbi[0]h_dark_en
	{0x65a01,0xc0},//h_dark_th[6:0]
	//lowcurve
	{0x1C4C0,0x03},
	{0x1C4C1,0x05},
	{0x1C4C2,0x08},
	{0x1C4C3,0x0d},
	{0x1C4C4,0x12},
	{0x1C4C5,0x18},
	{0x1C4C6,0x1f},
	{0x1C4C7,0x27},
	{0x1C4C8,0x30},
	{0x1C4C9,0x3b},
	{0x1C4CA,0x49},
	{0x1C4CB,0x5a},
	{0x1C4CC,0x71},
	{0x1C4CD,0x97},
	{0x1C4CE,0xc8},
	//highcurve
	{0x1d985,0x5},//highgaincurve00
	{0x1d986,0xa},//highgaincurve01
	{0x1d987,0xf},
	{0x1d988,0x14},
	{0x1d989,0x19},
	{0x1d98a,0x1e},
	{0x1d98b,0x24},
	{0x1d98c,0x2b},
	{0x1d98d,0x33},
	{0x1d98e,0x3d},
	{0x1d98f,0x4a},
	{0x1d990,0x5b},
	{0x1d991,0x72},
	{0x1d992,0x97},
	{0x1d993,0xc8},//highgaincurve15
	/*EDR*/
	{0x1c4d4,0x00},//EDRscale,disableedrwith0
	{0x1c4d5,0x00},//EDRscale,disableedrwith0
	{0x1c4cf,0xff},//disableedrwith0xff
	/*LENC*/
	{0x65102,0x07},
	{0x65103,0x87},
	{0x65104,0x0a},
	{0x65105,0x0a},
	{0x65106,0x0a},
	{0x65107,0x0a},
	{0x65108,0x06},
	{0x65109,0xb1},
	{0x1c244,0x01},//lenscorrectionautogainenable
	{0x1c245,0x03},//lensswtichbyonlinestat
	{0x1d9c6,0x00},
	//LENSonline
	{0x1c574,0x00},//lowgain
	{0x1c575,0x40},
	{0x1c576,0x00},//highgain
	{0x1c577,0xa0},
	{0x1c578,0x40},//minlensapplyratio
	{0x65206,0xfa},//RG
	{0x65207,0x06},
	{0x65208,0xfa},//BG
	{0x65209,0x06},
	{0x1d9c0,0x00},//switchtodaythres
	{0x1d9c1,0xa0},
	{0x1d9c2,0xff},//switchtoTL84thres
	{0x1d9c3,0x90},
	{0x1c579,0x01},//autolensonline
	{0x1d9be,0x00},//outdoorthreshold2bytes
	{0x1d9bf,0x80},
	{0x1d9c7,0x20},//outdoorthresholdratio,(x+32)/32
	{0x1d9c8,0xfc},//outdoorRG
	{0x1d9ca,0x04},
	{0x1d9cc,0x00},//outdoorBG
	{0x1d9ce,0x00},
	{0x1d9c9,0xfa},//indoorRG
	{0x1d9cb,0x06},
	{0x1d9cd,0xfc},//indoorBG
	{0x1d9cf,0x04},
	//forluminacecorretcion,theluminancecompensationamplitudecanchangewiththesensorgaqin
	{0x1c248,0x40},//maxlenscorrectionratio
	{0x1c24a,0x20},//minlenscorrectionratio
	{0x1c24c,0x00},//gainthreshold0[15:8]
	{0x1c24d,0x20},//gainthreshold0[7:0]
	{0x1c24e,0x00},//gainthreshold1[15:8]
	{0x1c24f,0x80},//gainthreshold1[7:0]
	//lensprofileswitch
	//addshadingparameteraccording0x1c728~0x1c72d,b_gain/r_gain*256
	{0x1c254,0x01},//D
	{0x1c255,0x04},
	{0x1c256,0x01},//D-cwf4150
	{0x1c257,0x1a},
	{0x1c258,0x01},//cwf-a3100k
	{0x1c259,0xba},
	{0x1c25a,0x01},//a2750k
	{0x1c25b,0xea},
	/*OVISPLENCsettingforD65LongExposure(HDR/3D)*/
	//Ychannelre-backtooldversion(donotplus8)20120821byy00215412
	//DLight
	{0x1c264,0x3f},
	{0x1c265,0x3a},
	{0x1c266,0x2a},
	{0x1c267,0x29},
	{0x1c268,0x38},
	{0x1c269,0x3f},
	{0x1c26a,0x25},
	{0x1c26b,0x14},
	{0x1c26c,0x0d},
	{0x1c26d,0x0c},
	{0x1c26e,0x13},
	{0x1c26f,0x20},
	{0x1c270,0x14},
	{0x1c271,0x07},
	{0x1c272,0x00},
	{0x1c273,0x00},
	{0x1c274,0x06},
	{0x1c275,0x12},
	{0x1c276,0x16},
	{0x1c277,0x06},
	{0x1c278,0x00},
	{0x1c279,0x00},
	{0x1c27a,0x06},
	{0x1c27b,0x13},
	{0x1c27c,0x21},
	{0x1c27d,0x12},
	{0x1c27e,0x0b},
	{0x1c27f,0x0a},
	{0x1c280,0x12},
	{0x1c281,0x1d},
	{0x1c282,0x3f},
	{0x1c283,0x37},
	{0x1c284,0x28},
	{0x1c285,0x28},
	{0x1c286,0x35},
	{0x1c287,0x3f},
	{0x1c288,0x1f},
	{0x1c289,0x1c},
	{0x1c28a,0x1b},
	{0x1c28b,0x1d},
	{0x1c28c,0x20},
	{0x1c28d,0x1c},
	{0x1c28e,0x1c},
	{0x1c28f,0x1e},
	{0x1c290,0x1d},
	{0x1c291,0x1d},
	{0x1c292,0x1b},
	{0x1c293,0x1f},
	{0x1c294,0x21},
	{0x1c295,0x20},
	{0x1c296,0x1c},
	{0x1c297,0x1c},
	{0x1c298,0x1d},
	{0x1c299,0x1e},
	{0x1c29a,0x1d},
	{0x1c29b,0x1c},
	{0x1c29c,0x1e},
	{0x1c29d,0x1c},
	{0x1c29e,0x1a},
	{0x1c29f,0x1c},
	{0x1c2a0,0x1f},
	{0x1c2a1,0x24},
	{0x1c2a2,0x24},
	{0x1c2a3,0x24},
	{0x1c2a4,0x23},
	{0x1c2a5,0x26},
	{0x1c2a6,0x25},
	{0x1c2a7,0x22},
	{0x1c2a8,0x22},
	{0x1c2a9,0x22},
	{0x1c2aa,0x26},
	{0x1c2ab,0x24},
	{0x1c2ac,0x21},
	{0x1c2ad,0x1f},
	{0x1c2ae,0x21},
	{0x1c2af,0x26},
	{0x1c2b0,0x25},
	{0x1c2b1,0x22},
	{0x1c2b2,0x22},
	{0x1c2b3,0x22},
	{0x1c2b4,0x26},
	{0x1c2b5,0x22},
	{0x1c2b6,0x23},
	{0x1c2b7,0x22},
	{0x1c2b8,0x22},
	{0x1c2b9,0x24},
	//U30
	{0x1c2ba,0x3f},
	{0x1c2bb,0x3a},
	{0x1c2bc,0x2a},
	{0x1c2bd,0x29},
	{0x1c2be,0x38},
	{0x1c2bf,0x3f},
	{0x1c2c0,0x26},
	{0x1c2c1,0x14},
	{0x1c2c2,0x0d},
	{0x1c2c3,0x0c},
	{0x1c2c4,0x13},
	{0x1c2c5,0x21},
	{0x1c2c6,0x15},
	{0x1c2c7,0x07},
	{0x1c2c8,0x00},
	{0x1c2c9,0x00},
	{0x1c2ca,0x06},
	{0x1c2cb,0x12},
	{0x1c2cc,0x16},
	{0x1c2cd,0x06},
	{0x1c2ce,0x00},
	{0x1c2cf,0x00},
	{0x1c2d0,0x05},
	{0x1c2d1,0x13},
	{0x1c2d2,0x21},
	{0x1c2d3,0x13},
	{0x1c2d4,0x0b},
	{0x1c2d5,0x0a},
	{0x1c2d6,0x12},
	{0x1c2d7,0x1e},
	{0x1c2d8,0x3f},
	{0x1c2d9,0x37},
	{0x1c2da,0x28},
	{0x1c2db,0x28},
	{0x1c2dc,0x34},
	{0x1c2dd,0x3f},
	{0x1c2de,0x1e},
	{0x1c2df,0x1c},
	{0x1c2e0,0x1a},
	{0x1c2e1,0x1d},
	{0x1c2e2,0x1e},
	{0x1c2e3,0x1b},
	{0x1c2e4,0x1b},
	{0x1c2e5,0x1e},
	{0x1c2e6,0x1c},
	{0x1c2e7,0x1c},
	{0x1c2e8,0x19},
	{0x1c2e9,0x1f},
	{0x1c2ea,0x22},
	{0x1c2eb,0x20},
	{0x1c2ec,0x1a},
	{0x1c2ed,0x1a},
	{0x1c2ee,0x1c},
	{0x1c2ef,0x1e},
	{0x1c2f0,0x1c},
	{0x1c2f1,0x1b},
	{0x1c2f2,0x1d},
	{0x1c2f3,0x1b},
	{0x1c2f4,0x18},
	{0x1c2f5,0x1b},
	{0x1c2f6,0x1e},
	{0x1c2f7,0x22},
	{0x1c2f8,0x22},
	{0x1c2f9,0x20},
	{0x1c2fa,0x21},
	{0x1c2fb,0x25},
	{0x1c2fc,0x22},
	{0x1c2fd,0x20},
	{0x1c2fe,0x20},
	{0x1c2ff,0x20},
	{0x1c300,0x23},
	{0x1c301,0x21},
	{0x1c302,0x20},
	{0x1c303,0x20},
	{0x1c304,0x20},
	{0x1c305,0x22},
	{0x1c306,0x22},
	{0x1c307,0x20},
	{0x1c308,0x21},
	{0x1c309,0x20},
	{0x1c30a,0x22},
	{0x1c30b,0x21},
	{0x1c30c,0x20},
	{0x1c30d,0x1e},
	{0x1c30e,0x20},
	{0x1c30f,0x22},
	/*AWB*/
	{0x66201,0x52},//
	{0x66203,0x14},//crop window
    {0x66211,0xe8},//awbtoplimit
    {0x66212,0x04},//awbbottomlimit
        /*Curve awb*/
	{0x1c190,0x02},//1,CT; 2, Curve
	{0x1d8c1,0x00},//Curve AWB Options
	{0x1d8e0,0x04},//min pixels
	{0x1d8e1,0x00},
	{0x66285,0x00},//minvalue
	{0x66286,0x09},
	{0x66287,0x03},//maxvalue
	{0x66288,0xe0},
	//dynamicmap
	{0x1d8dc,0x00},//nCurveAWBBrThres0
	{0x1d8dd,0x50},
	{0x1d8db,0x40},//nCurveAWBBrRatio0(x+32)/32
	{0x1d8e6,0x02},//nCurveAWBBrThres1
	{0x1d8e7,0x81},
	{0x1d8c6,0x20},//nCurveAWBBrRatio1(x+32)/32
//X_OFF
{0x66280, 0xfd},
{0x66281, 0xe4},
//Y_OFF
{0x66282, 0xfd},
{0x66283, 0xdd},
//KX
{0x1d8e2, 0x00},
{0x1d8e3, 0x29},
//KY
{0x1d8e4, 0x00},
{0x1d8e5, 0x0f},
//LowMap
{0x1d800, 0x00},
{0x1d801, 0x00},
{0x1d802, 0x00},
{0x1d803, 0x00},
{0x1d804, 0x00},
{0x1d805, 0x00},
{0x1d806, 0x00},
{0x1d807, 0x00},
{0x1d808, 0x00},
{0x1d809, 0x00},
{0x1d80a, 0x00},
{0x1d80b, 0x11},
{0x1d80c, 0x11},
{0x1d80d, 0x01},
{0x1d80e, 0x00},
{0x1d80f, 0x00},
{0x1d810, 0x00},
{0x1d811, 0x00},
{0x1d812, 0x20},
{0x1d813, 0x22},
{0x1d814, 0x22},
{0x1d815, 0x22},
{0x1d816, 0x00},
{0x1d817, 0x00},
{0x1d818, 0x00},
{0x1d819, 0x00},
{0x1d81a, 0x44},
{0x1d81b, 0x44},
{0x1d81c, 0x44},
{0x1d81d, 0x44},
{0x1d81e, 0x02},
{0x1d81f, 0x00},
{0x1d820, 0x00},
{0x1d821, 0x40},
{0x1d822, 0x44},
{0x1d823, 0x44},
{0x1d824, 0x44},
{0x1d825, 0x44},
{0x1d826, 0x24},
{0x1d827, 0x00},
{0x1d828, 0x00},
{0x1d829, 0x66},
{0x1d82a, 0x66},
{0x1d82b, 0x66},
{0x1d82c, 0x66},
{0x1d82d, 0x66},
{0x1d82e, 0x44},
{0x1d82f, 0x00},
{0x1d830, 0x00},
{0x1d831, 0x66},
{0x1d832, 0x66},
{0x1d833, 0x66},
{0x1d834, 0x66},
{0x1d835, 0x66},
{0x1d836, 0x66},
{0x1d837, 0x00},
{0x1d838, 0x00},
{0x1d839, 0x88},
{0x1d83a, 0x88},
{0x1d83b, 0x88},
{0x1d83c, 0x88},
{0x1d83d, 0x88},
{0x1d83e, 0x68},
{0x1d83f, 0x00},
{0x1d840, 0x00},
{0x1d841, 0x88},
{0x1d842, 0xa8},
{0x1d843, 0xaa},
{0x1d844, 0xaa},
{0x1d845, 0xaa},
{0x1d846, 0x8a},
{0x1d847, 0x00},
{0x1d848, 0x00},
{0x1d849, 0xa0},
{0x1d84a, 0xaa},
{0x1d84b, 0xdd},
{0x1d84c, 0xff},
{0x1d84d, 0xff},
{0x1d84e, 0xad},
{0x1d84f, 0x00},
{0x1d850, 0x00},
{0x1d851, 0x00},
{0x1d852, 0xaa},
{0x1d853, 0xfd},
{0x1d854, 0xff},
{0x1d855, 0xff},
{0x1d856, 0xaf},
{0x1d857, 0x00},
{0x1d858, 0x00},
{0x1d859, 0x00},
{0x1d85a, 0x80},
{0x1d85b, 0xaa},
{0x1d85c, 0xff},
{0x1d85d, 0xaf},
{0x1d85e, 0x0a},
{0x1d85f, 0x00},
{0x1d860, 0x00},
{0x1d861, 0x00},
{0x1d862, 0x00},
{0x1d863, 0x22},
{0x1d864, 0x22},
{0x1d865, 0x22},
{0x1d866, 0x02},
{0x1d867, 0x00},
{0x1d868, 0x00},
{0x1d869, 0x00},
{0x1d86a, 0x00},
{0x1d86b, 0x00},
{0x1d86c, 0x00},
{0x1d86d, 0x00},
{0x1d86e, 0x00},
{0x1d86f, 0x00},
{0x1d870, 0x00},
{0x1d871, 0x00},
{0x1d872, 0x00},
{0x1d873, 0x00},
{0x1d874, 0x00},
{0x1d875, 0x00},
{0x1d876, 0x00},
{0x1d877, 0x00},
{0x1d878, 0x00},
{0x1d879, 0x00},
{0x1d87a, 0x00},
{0x1d87b, 0x00},
{0x1d87c, 0x00},
{0x1d87d, 0x00},
{0x1d87e, 0x00},
{0x1d87f, 0x00},
//MiddleMask
{0x1d880, 0x00},
{0x1d881, 0x00},
{0x1d882, 0x00},
{0x1d883, 0x00},
{0x1d884, 0x00},
{0x1d885, 0x00},
{0x1d886, 0x00},
{0x1d887, 0x00},
{0x1d888, 0x00},
{0x1d889, 0x00},
{0x1d88a, 0x00},
{0x1d88b, 0x00},
{0x1d88c, 0x00},
{0x1d88d, 0x00},
{0x1d88e, 0x80},
{0x1d88f, 0x1f},
{0x1d890, 0xc0},
{0x1d891, 0x3f},
{0x1d892, 0xc0},
{0x1d893, 0x3f},
{0x1d894, 0xc0},
{0x1d895, 0x3f},
{0x1d896, 0xc0},
{0x1d897, 0x1f},
{0x1d898, 0x00},
{0x1d899, 0x00},
{0x1d89a, 0x00},
{0x1d89b, 0x00},
{0x1d89c, 0x00},
{0x1d89d, 0x00},
{0x1d89e, 0x00},
{0x1d89f, 0x00},
//HighMask
{0x1d8a0, 0x00},
{0x1d8a1, 0x00},
{0x1d8a2, 0x00},
{0x1d8a3, 0x00},
{0x1d8a4, 0x00},
{0x1d8a5, 0x00},
{0x1d8a6, 0x00},
{0x1d8a7, 0x00},
{0x1d8a8, 0x00},
{0x1d8a9, 0x00},
{0x1d8aa, 0x00},
{0x1d8ab, 0x00},
{0x1d8ac, 0x00},
{0x1d8ad, 0x00},
{0x1d8ae, 0x80},
{0x1d8af, 0x0f},
{0x1d8b0, 0xc0},
{0x1d8b1, 0x1f},
{0x1d8b2, 0xc0},
{0x1d8b3, 0x1f},
{0x1d8b4, 0xc0},
{0x1d8b5, 0x1f},
{0x1d8b6, 0x00},
{0x1d8b7, 0x00},
{0x1d8b8, 0x00},
{0x1d8b9, 0x00},
{0x1d8ba, 0x00},
{0x1d8bb, 0x00},
{0x1d8bc, 0x00},
{0x1d8bd, 0x00},
{0x1d8be, 0x00},
{0x1d8bf, 0x00},
	{0x1c194,0x01},
	{0x1c195,0xbf},
	/*awbshift*/
	{0x1d8e8,0x05},//0x01:EnableAWBShift,0x02:EnableBrShift,0x04:enableMultipleMap,turnoffawbshiftfortest
	/*newAWBshift*/
	{0x1d9e0,0x01},//Enablenewawbshift
	{0x1d8fa,0x00},//nAWBShiftBrThres1
	{0x1d8fb,0xf0},
	{0x1d8fc,0x0a},//nAWBShiftBrThres2
	{0x1d8fd,0x00},
	{0x1d8fe,0x30},//nAWBShiftBrThres3
	{0x1d8ff,0x00},
	{0x1d9ea,0xd0},//nAWBShiftBrThres4
	{0x1d9eb,0x00},
	{0x1d9e8,0x0b},//nAWBShiftLowBrightThres
	{0x1d9e9,0x28},//nAWBShiftHighBrightThres
	//awbshiftthreshold,refertoregr_gain/b_gain*128��bgain:0x1c728~0x1c729,rgain:0x1c72c~0x1c72d
	{0x1d9e2,0x2b},//HCT
	{0x1d9e1,0x3a},//ACT
	{0x1d8f1,0x67},//TLCT
	{0x1d8f2,0x6e},//cwfCT
	{0x1d8f3,0xae},//DCT
	{0x1da0c,0x7c},//BgainforHHighlight
	{0x1da0d,0x7c},//BgainforH
	{0x1da0e,0x7c},//BgainforH
	{0x1da0f,0x7c},//BgainforHlowlight
	{0x1da10,0x80},//RgainforHHighlight
	{0x1da11,0x80},//RgainforH
	{0x1da12,0x80},//RgainforH
	{0x1da13,0x80},//RgainforHlowlight
	
	{0x1da04,0x80},//BgainforAHighlight
	{0x1da05,0x80},//BgainforA
	{0x1da06,0x80},//BgainforA
	{0x1da07,0x80},//BgainforAlowlight
	{0x1da08,0x80},//RgainforAHighlight
	{0x1da09,0x80},//RgainforA
	{0x1da0a,0x80},//RgainforA
	{0x1da0b,0x80},//RgainforAlowlight
	
	{0x1d9ec,0x80},//BgainforTLHighlight
	{0x1d9ed,0x80},//BgainforTL
	{0x1d9ee,0x80},//BgainforTL
	{0x1d9ef,0x80},//BgainforTLlowlight
	{0x1d9f0,0x80},//RgainforTLHighlight
	{0x1d9f1,0x80},//RgainforTL
	{0x1d9f2,0x80},//RgainforTL
	{0x1d9f3,0x80},//RgainforTLlowlight
	{0x1d9f4,0x80},//BgainforcwfHighlight
	{0x1d9f5,0x80},//Bgainforcwf
	{0x1d9f6,0x80},//Bgainforcwf
	{0x1d9f7,0x80},//Bgainforcwflowlight
	{0x1d9f8,0x80},//RgainforcwfHighlight
	{0x1d9f9,0x80},//Rgainforcwf
	{0x1d9fa,0x80},//Rgainforcwf
	{0x1d9fb,0x80},//Rgainforcwflowlight
	{0x1d9fc,0x80},//BgainforDHighlight
	{0x1d9fd,0x80},//BgainforD
	{0x1d9fe,0x80},//BgainforD
	{0x1d9ff,0x80},//BgainforDlowlight
	{0x1da00,0x80},//RgainforDHighlight
	{0x1da01,0x80},//RgainforD
	{0x1da02,0x80},//RgainforD
	{0x1da03,0x80},//RgainforDlowlight

	/*ccm*/
	//addccmdetectparameterrefertoregg_gain/r_gain*256��bgain:0x1c728~0x1c729,rgain:0x1c72c~0x1c72d
	{0x1c1c8,0x01},//centerCT,CWF
	{0x1c1c9,0x11},
	{0x1c1cc,0x00},//daylight
	{0x1c1cd,0xE2},
	{0x1c1d0,0x01},//a
	{0x1c1d1,0x90},
	/*Colormatrix*/
	{0x1c1d8,0x01},
	{0x1c1d9,0xBB},
	{0x1c1da,0xFF},
	{0x1c1db,0x72},
	{0x1c1dc,0xFF},
	{0x1c1dd,0xD3},
	{0x1c1de,0x00},
	{0x1c1df,0x02},
	{0x1c1e0,0x01},
	{0x1c1e1,0x7F},
	{0x1c1e2,0xFF},
	{0x1c1e3,0x7F},
	{0x1c1e4,0xFF},
	{0x1c1e5,0x03},
	{0x1c1e6,0xFE},
	{0x1c1e7,0xE5},
	{0x1c1e8,0x03},
	{0x1c1e9,0x18},
	//cmxleftdelta,daylight
	{0x1c1fc,0xFF},
	{0x1c1fd,0xEF},
	{0x1c1fe,0xFF},
	{0x1c1ff,0xE8},
	{0x1c200,0x00},
	{0x1c201,0x27},
	{0x1c202,0xFF},
	{0x1c203,0xE2},
	{0x1c204,0xFF},
	{0x1c205,0xE3},
	{0x1c206,0x00},
	{0x1c207,0x3B},
	{0x1c208,0x00},
	{0x1c209,0x98},
	{0x1c20a,0x00},
	{0x1c20b,0x76},
	{0x1c20c,0xFE},
	{0x1c20d,0xF0},
	//cmxrightdelta,alight
	{0x1c220,0x00},
	{0x1c221,0x7D},
	{0x1c222,0xFF},
	{0x1c223,0x7F},
	{0x1c224,0x00},
	{0x1c225,0x03},
	{0x1c226,0x00},
	{0x1c227,0x1B},
	{0x1c228,0xFF},
	{0x1c229,0xB9},
	{0x1c22a,0x00},
	{0x1c22b,0x2B},
	{0x1c22c,0x00},
	{0x1c22d,0xE0},
	{0x1c22e,0x00},
	{0x1c22f,0xE6},
	{0x1c230,0xFE},
	{0x1c231,0x3A},
	/*autouvsaturation*/
	{0x1c4e8,0x01},//Enable
	{0x1c4e9,0xb0},//gainthreshold140-->0b
	{0x1c4ea,0xf7},//gainthreshold278-->0d
	{0x1c4eb,0x70},//keepbackfornewcmx0310
	{0x1c4ec,0x6c},//keepbackfornewcmx0310
	/*De-noise*/
	{0x65c00,0x01},//UVDe-noise:gain1X
	{0x65c01,0x02},//gain2X
	{0x65c02,0x04},//gain4X
	{0x65c03,0x06},//gain8X
	{0x65c04,0x0a},//gain16X
	{0x65c05,0x12},//gain32X
	//2.2Raw&UVDenoiseSetting
	{0x67300,0x08},//[6:0]r_s0_sigma_0_i-gain-1X
	{0x67301,0x0c},//[6:0]r_s0_sigma_1_i-gain-2X
	{0x67302,0x12},//[6:0]r_s0_sigma_2_i-gain-4X
	{0x67303,0x1e},//[6:0]r_s0_sigma_3_i-gain-8X
	{0x67304,0x20},//[6:0]r_s0_sigma_4_i-gain-16X
	{0x67305,0x2a},//[6:0]r_s0_sigma_5_i-gain-32X
	{0x67306,0x0c},//[5:0]r_s0_gsl_0_i-gain-1X
	{0x67307,0x0c},//[5:0]r_s0_gsl_1_i-gain-2X
	{0x67308,0x0c},//[5:0]r_s0_gsl_2_i-gain-4X
	{0x67309,0x14},//[5:0]r_s0_gsl_3_i-gain-8X
	{0x6730a,0x18},//[5:0]r_s0_gsl_4_i-gain-16X
	{0x6730b,0x18},//[5:0]r_s0_gsl_5_i-gain-32X
	{0x6730c,0x06},//[5:0]r_s0_rbsl_0_i
	{0x6730d,0x06},//[5:0]r_s0_rbsl_1_i
	{0x6730e,0x06},//[5:0]r_s0_rbsl_2_i
	{0x6730f,0x0a},//[5:0]r_s0_rbsl_3_i
	{0x67310,0x0c},//[5:0]r_s0_rbsl_4_i
	{0x67311,0x0c},//[5:0]r_s0_rbsl_5_i
	
	//threshold of PL each corresponding to one of 1x, 2x, �� 32x sensor gain, when THY = 0x04
	{0x6732a,0x20}, //gain1X
	{0x6732b,0x20}, //gain2X
	{0x6732c,0x20}, //gain4X
	{0x6732d,0x20}, //gain8X
	{0x6732e,0x20}, //gain16X
	{0x6732f,0x20}, //gsin32X
	
	{0x67330,0x12}, //gain1X	//when THY = 0x14
	{0x67331,0x12}, //gain2X
	{0x67332,0x20}, //gain4X
	{0x67333,0x20}, //gain8X
	{0x67334,0x20}, //gain16X
	{0x67335,0x20}, //gsin32X
	
	{0x67336,0x0a}, //gain1X	//when THY = 0x24
	{0x67337,0x0c}, //gain2X
	{0x67338,0x20}, //gain4X
	{0x67339,0x20}, //gain8X
	{0x6733a,0x20}, //gain16X
	{0x6733b,0x20}, //gsin32X
	
	{0x6733c,0x08}, //gain1X	//when THY = 0x44
	{0x6733d,0x0a}, //gain2X
	{0x6733e,0x16}, //gain4X
	{0x6733f,0x20}, //gain8X
	{0x67340,0x20}, //gain16X	
	{0x67341,0x20}, //gsin32X
	/*sharpeness*/
	{0x65600,0x00},
	{0x65601,0x10},//min_gain
	{0x65602,0x00},
	{0x65603,0xa0},//max_gain
	{0x65608,0x08},//unsharpenmask[0]
	{0x65609,0x20},//unsharpenmask[1]
	{0x6560c,0x00},//min_sharpen--maxgain--sharpenlevel[4:0]
	{0x6560d,0x12},//max_sharpen--mingain[5:0]
	{0x6560e,0x10},//MinSharpenTp--mingain--softthreshold[5:0]
	{0x6560f,0x60},//MaxSharpenTp--maxgain
	{0x65610,0x20},//MinSharpenTm--mingain[5:0]
	{0x65611,0x60},//MaxSharpenTm--maxgain
	{0x65613,0x10},//SharpenAlpha
	{0x65615,0x08},//HFreq_thre
	{0x65617,0x06},//HFreq_coef[3:0]
	/*dpc*/
	{0x65409,0x08},
	{0x6540a,0x08},
	{0x6540b,0x08},
	{0x6540c,0x08},
	{0x6540d,0x0c},
	{0x6540e,0x08},
	{0x6540f,0x08},
	{0x65410,0x08},
	{0x65408,0x0b},
	/*AF*/
	{0x1cd0a,0x00},//shouldclose
#endif
	{0x0,0x0},
},

/*ae_params_s*/
{
	/*ae_target_s*/
	{
		{0xf,0x11},
		{0x1d,0x1f},
		{0x32,0x42},/*standardAEtarget*/
		{0x44,0x48},
		{0x7e,0x80},
	},

	/*ae_win_params*/
	{

		/*win2x2*/
		{80,80},
		/*win3x3*/
		{{48,48},{48,48},{100,100},},
		/*roi_enable*/
		false,
		/*default_stat_winweights*/
		{1,1,1,1,2,2,2,2,16,2,2,2,2},

		4,
		/*enhanced_stat_win*/
		{1,1,1,1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},

		8,
	},
	/*max_expo_gap*/
	14,
	/*iso_max*/
	2400,
	/*iso_min*/
	100,

	{
		/*low2mid*/
		0x70,
		/*mid2high*/
		0x40,
		/*high2mid*/
		0xe0,
		/*mid2low*/
		0xe0
	},

	{
		/* low2mid manual iso*/
		310,
		/* mid2high manual iso*/
		180,
		/* high2mid manual iso*/
		120,
		/* mid2low manual iso*/
		120
	},

	{
		/*preview_fps_max*/
		30,
		/*preview_fps_middle*/
		22,//15
		/*preview_fps_min*/
		15,//10,
		/*capture_fps_min*/
		10
	},

	/*expo_night*/
	5,
	/*expo_action*/
	100,
	/*cap_expo_table*/
	{
		{30,100},/*maxexpois3band(50Hz),4band(60Hz)*/
		//{23,180},/*maxexpois4band(50Hz),5band(60Hz)*/
		{20,200},/*maxexpois5band(50Hz),6band(60Hz)*/

		/*BelowisLowercaptureframerate(orlargerexposure)thannormalframerate.*/
		{14,600},/*maxexpois7band(50Hz),8band(60Hz)*/
		{10,800},/*maxexpois10band(50Hz),12band(60Hz)*/
		//{8,8000},/*maxexpois12band(50Hz),15band(60Hz)*/
		{0,0},
		{0,0},
		{0,0},
		{0,0},
		{0,0},
		{0,0},
	},
	/*ev_numerator*/
	635,
	/*u32ev_denominator*/
	1000,

},

/*af_param*/
{
	/*focus_area*/
	{
		/*percent_w*/
		25,
		/*percent_w*/
		20,
		/*max_zoom_ratio*/
		0x200,
		/*min_height_ratio*/
		5,
		/*weight[25]*/
		{0},
	},
	/*focus_algo_s*/
	{
		/*contrast_stat_mode*/
		YUV_EDGE_STAT,
		/*contrast_threshold*/
		{0x04,0x10,0x80,0x140},
		/*param_judge*/
		{0x18,0x08,0xc0,0x04,95},
		/*infinity_hold_frames*/
		2,
		/*rewind_hold_frames*/
		1,
		/*try_dir_range*/
		5,
		/*param_caf_trigger*/
		{3,3,4,10,0x28,25,16,100,0xc,4,4,2000,2,6,15,6,4,0x40},
		/*calc_coarse_size*/
		(1280*720),
	},
},

/*manual_wb_param*/
{
	/*b_gaigb_gaingr_gainr_gain*/
	{0x0000,0x0000,0x0000,0x0000},/*AWBnotcareaboutit*/
	{0x0118,0x0080,0x0080,0x0084},/*INCANDESCENT2800K*/
	{0x00e8,0x0080,0x0080,0x00bd},/*FLUORESCENT4200K*/
	{0x00a0,0x00a0,0x00a0,0x00a0},/*WARM_FLUORESCENT,y36721todo*/
	{0x00b5,0x0080,0x0080,0x00c6},/*DAYLIGHT5000K*/
	{0x0091,0x0080,0x0080,0x00df},/*CLOUDY_DAYLIGHT6500K*/
	{0x00a0,0x00a0,0x00a0,0x00a0},/*TWILIGHT,y36721todo*/
	{0x0168,0x0080,0x0080,0x0060},/*CANDLELIGHT,2300K*/
},


/*rcc*/
{
	/*rcc_enable*/
	false,
	/*frame_interval*/
	8,
	/*detect_range*/
	50,
	/*rect_row_num*/
	5,
	/*rect_col_num*/
	5,
	/*preview_width_high*/
	960,
	/*preview_width_low*/
	320,
	/*uv_resample_high*/
	8,
	/*uv_resample_middle*/
	4,
	/*uv_resample_low*/
	2,
	/*refbin_low*/
	0x80,
	/*refbin_high*/
	0xf0,
	/*v_th_high*/
	160,
	/*v_th_low*/
	130,
},


/*sharpness_cfg*/
{
	{
	/*preview_shapness*/
	0x30,//hefeimodifyforimx134sensor
	/*cap_shapness*/
	0x18,
	},
},


/*dns*/
{
	/*zsl_off*/
		/*raw_dns_coff*/
		 /*raw_dns_coff*/
	   {{0x08,0x0c,0x12,0x1e,0x20,0x2a},{0x0c,0x0c,0x0c,0x14,0x18,0x18},{0x06,0x06,0x06,0x0a,0x0c,0x0c}},
	   {{0x08,0x0c,0x12,0x1e,0x20,0x2a},{0x0c,0x0c,0x0c,0x14,0x18,0x18},{0x06,0x06,0x06,0x0a,0x0c,0x0c}},
	   {{0x08,0x0c,0x12,0x1e,0x20,0x2a},{0x0c,0x0c,0x0c,0x14,0x18,0x18},{0x06,0x06,0x06,0x0a,0x0c,0x0c}},
		/*g_dns_caputure_1_band*/
		0x23,
		/*g_dns_flash_1_band*/
		0x23,
		{0x03,0x05,0x08,0x1f,0x1f,0x1f},
		{0x03,0x05,0x08,0x1f,0x1f,0x1f},
		{0x03,0x05,0x08,0x1f,0x1f,0x1f},
},


/*scene_param:uv_saturation*/
{
	0x74,/*CAMERA_SCENE_AUTO*/
	0x80,/*CAMERA_SCENE_ACTION*/
	0x90,/*CAMERA_SCENE_PORTRAIT*/
	0x98,/*CAMERA_SCENE_LANDSPACE*/
	0x80,/*CAMERA_SCENE_NIGHT*/
	0x90,/*CAMERA_SCENE_NIGHT_PORTRAIT*/
	0x70,/*CAMERA_SCENE_THEATRE*/
	0x80,/*CAMERA_SCENE_BEACH*/
	0x80,/*CAMERA_SCENE_SNOW*/
	0x80,/*CAMERA_SCENE_SUNSET*/
	0x80,/*CAMERA_SCENE_STEADYPHOTO*/
	0x80,/*CAMERA_SCENE_FIREWORKS*/
	0x80,/*CAMERA_SCENE_SPORTS*/
	0x80,/*CAMERA_SCENE_PARTY*/
	0x80,/*CAMERA_SCENE_CANDLELIGHT*/
	0x80,/*CAMERA_SCENE_BARCODE*/
	0x80,/*CAMERA_SCENE_FLOWERS*/
},

/*flash_param*/
{
/*gain*/
{0xc2,0x80,0x80,0xe8},

/*aceawb_step*/
{0x06,0x18,0x18,0x10,0x0c},
/*videoflash_level*/
LUM_LEVEL2,
/*assistant_af_params*/
{0xA0,0x10,0x30,30,LUM_LEVEL1},
/*flash_capture*/
{0x100,LUM_LEVEL2,LUM_LEVEL5,0x04,0x30,30,0xe0,0x20,0x140},
},

#endif//__EFFECT_OV5648_FOXCONN_CHM_H__

