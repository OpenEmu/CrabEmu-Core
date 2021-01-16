/*
 Copyright (c) 2009, OpenEmu Team

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
     * Neither the name of the OpenEmu Team nor the
       names of its contributors may be used to endorse or promote products
       derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY OpenEmu Team ''AS IS'' AND ANY
 EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL OpenEmu Team BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "SMSGameCore.h"
#import <OpenEmuBase/OERingBuffer.h>
#import <OpenGL/gl.h>
#import "OESMSSystemResponderClient.h"
#import "OEGGSystemResponderClient.h"
#import "OESG1000SystemResponderClient.h"
#import "OEColecoVisionSystemResponderClient.h"

#include "sms.h"
#include "smsmem.h"
#include "sound.h"
#include "smsvdp.h"
#include "smsz80.h"
#include "rom.h"
#include "colecovision.h"
#include "colecomem.h"
#include "cheats.h"
#include "console.h"

#if MAC_OS_X_VERSION_MIN_REQUIRED < __MAC_10_13
#include "fmemopen/fmemopen.h"
#include "fmemopen/open_memstream.h"
#endif

#define SAMPLERATE 44100

@interface SMSGameCore () <OESMSSystemResponderClient, OEGGSystemResponderClient, OESG1000SystemResponderClient, OEColecoVisionSystemResponderClient>
{
    NSLock        *bufLock;
    BOOL           paused;
    NSURL         *romFile;
    NSMutableDictionary *cheatList;
}
@end

@implementation SMSGameCore

// Global variables because the callbacks need to access them...
static OERingBuffer *ringBuffer;
console_t *cur_console;

- (id)init
{
    self = [super init];
    if(self != nil)
    {
        bufLock = [[NSLock alloc] init];
        cheatList = [[NSMutableDictionary alloc] init];
        ringBuffer = [self ringBufferAtIndex:0];
    }
    return self;
}

- (void)dealloc
{
    DLog(@"releasing/deallocating CrabEmu memory");

    cur_console->shutdown();
}

# pragma mark - Execution

- (BOOL)loadFileAtPath:(NSString *)path error:(NSError **)error
{
    romFile = [NSURL fileURLWithPath:path];
    int console = rom_detect_console(path.fileSystemRepresentation);
    DLog(@"Loaded File");
    //TODO: add choice NTSC/PAL
    if(console == CONSOLE_COLECOVISION)
    {
        NSString *biosPath = [[self biosDirectoryPath] stringByAppendingPathComponent:@"coleco.rom"];
        coleco_init(VIDEO_NTSC);
        coleco_mem_load_bios(biosPath.fileSystemRepresentation);
        coleco_mem_load_rom(path.fileSystemRepresentation);
    }
    else
    {
        NSData *dataObj = [NSData dataWithContentsOfFile:[path stringByStandardizingPath]];
        const void *data = [dataObj bytes];
        uint16_t offset = 0;
        int region = SMS_REGION_DOMESTIC;

        // Detect SMS ROM header
        if(memcmp(&data[0x1ff0], "TMR SEGA", 8) == 0)
            offset = 0x1ff0;
        else if (memcmp(&data[0x3ff0], "TMR SEGA", 8) == 0)
            offset = 0x3ff0;
        else if (memcmp(&data[0x7ff0], "TMR SEGA", 8) == 0)
            offset = 0x7ff0;

        if(offset)
        {
            // Set machine region
            switch (((char *)data)[offset + 0x0f] >> 4)
            {
                case 3: // SMS Japan
                    region = SMS_REGION_DOMESTIC;
                    break;
                case 4: // SMS Export
                    // Force system region to Japan if user locale is Japan and the cart is world/multi-region
                    region = [[self systemRegion] isEqualToString: @"Japan"] ? SMS_REGION_DOMESTIC : SMS_REGION_EXPORT;
                    break;
                case 5: // GG Japan
                case 6: // GG Export
                case 7: // GG International
                default:
                    region = SMS_REGION_DOMESTIC;
                    break;
            }
        }
        else
            // No header means Japan region
            region = SMS_REGION_DOMESTIC;

        sms_init(SMS_VIDEO_NTSC, region, 0); // 1 = VDP borders
        sms_mem_load_rom(path.fileSystemRepresentation, console);
        cur_console->frame(0);
    }

    if(cur_console->console_type != CONSOLE_COLECOVISION)
    {
        NSString *extensionlessFilename = [[romFile lastPathComponent] stringByDeletingPathExtension];
        NSURL *batterySavesDirectory = [NSURL fileURLWithPath:[self batterySavesDirectoryPath]];
        [[NSFileManager defaultManager] createDirectoryAtURL:batterySavesDirectory withIntermediateDirectories:YES attributes:nil error:nil];
        NSURL *saveFile = [batterySavesDirectory URLByAppendingPathComponent:[extensionlessFilename stringByAppendingPathExtension:@"sav"]];

        if([saveFile checkResourceIsReachableAndReturnError:nil] && sms_read_cartram_from_file(saveFile.path.fileSystemRepresentation) == 0)
            NSLog(@"CrabEmu: Loaded sram");
    }

    return YES;
}

- (void)executeFrame
{
    [bufLock lock];
    cur_console->frame(0);
    [bufLock unlock];
}

- (void)resetEmulation
{
    cur_console->soft_reset();
}

- (void)stopEmulation
{
    if(cur_console->console_type != CONSOLE_COLECOVISION)
    {
        NSString *extensionlessFilename = [[romFile lastPathComponent] stringByDeletingPathExtension];
        NSURL *batterySavesDirectory = [NSURL fileURLWithPath:[self batterySavesDirectoryPath]];
        NSURL *saveFile = [batterySavesDirectory URLByAppendingPathComponent:[extensionlessFilename stringByAppendingPathExtension:@"sav"]];

        cur_console->save_sram(saveFile.path.fileSystemRepresentation);
    }

    [super stopEmulation];
}

- (NSTimeInterval)frameInterval
{
    return 60;
}

# pragma mark - Video

- (OEIntSize)bufferSize
{
    uint32_t f_x, f_y;
    cur_console->frame_size(&f_x, &f_y);
    return OEIntSizeMake(f_x, f_y);
}

- (OEIntRect)screenRect
{
    uint32_t a_x, a_y, a_w, a_h;
    cur_console->active_size(&a_x, &a_y, &a_w, &a_h);
    return OEIntRectMake(a_x, a_y, a_w, a_h);
}

- (OEIntSize)aspectSize
{
    return OEIntSizeMake(cur_console->console_type == CONSOLE_GG ? 160 : 256 * (8.0/7.0), cur_console->console_type == CONSOLE_GG ? 144 : 192);
}

- (const void *)getVideoBufferWithHint:(void *)hint
{
    if (!hint) {
        return cur_console->framebuffer();
    }
    return smsvdp.framebuffer = (uint32*)hint;
}

- (GLenum)pixelFormat
{
    return GL_BGRA;
}

- (GLenum)pixelType
{
    return GL_UNSIGNED_INT_8_8_8_8_REV;
}

# pragma mark - Audio

- (double)audioSampleRate
{
    return SAMPLERATE;
}

- (NSUInteger)channelCount
{
    return 2;
}

# pragma mark - Save States

- (void)saveStateToFileAtPath:(NSString *)fileName completionHandler:(void (^)(BOOL, NSError *))block
{
    block(cur_console->save_state([fileName fileSystemRepresentation]) == 0, nil);
}

- (void)loadStateFromFileAtPath:(NSString *)fileName completionHandler:(void (^)(BOOL, NSError *))block
{
    if([[self systemIdentifier] isEqualToString:@"openemu.system.sg1000"])
    {
        cur_console->load_state([fileName fileSystemRepresentation]);
        block(YES, nil);
    }
    else
        block(cur_console->load_state([fileName fileSystemRepresentation]) == 0, nil);
}

- (NSData *)serializeStateWithError:(NSError **)outError
{
    void *bytes;
    size_t length;
    FILE *fp = open_memstream((char **)&bytes, &length);

    int status;
    if(cur_console->console_type == CONSOLE_COLECOVISION)
        status = coleco_write_state(fp);
    else
        status = sms_write_state(fp);

    if(status == 0) {
        fclose(fp);
        return [NSData dataWithBytesNoCopy:bytes length:length];
    }

    if(outError) {
        *outError = [NSError errorWithDomain:OEGameCoreErrorDomain code:OEGameCoreCouldNotSaveStateError userInfo:@{
            NSLocalizedDescriptionKey : @"Save state data could not be written",
            NSLocalizedRecoverySuggestionErrorKey : @"The emulator could not write the state data."
        }];
    }

    fclose(fp);
    free(bytes);
    return nil;
}

- (BOOL)deserializeState:(NSData *)state withError:(NSError **)outError
{
    const void *bytes = [state bytes];
    size_t length = [state length];

    FILE *fp = fmemopen((void *)bytes, length, "rb");

    int status;
    if(cur_console->console_type == CONSOLE_COLECOVISION)
        status = coleco_read_state(fp);
    else
        status = sms_read_state(fp);

    fclose(fp);

    if(status == 0)
        return YES;

    if(outError) {
        *outError = [NSError errorWithDomain:OEGameCoreErrorDomain code:OEGameCoreCouldNotLoadStateError userInfo:@{
            NSLocalizedDescriptionKey : @"The save state data could not be read",
            NSLocalizedRecoverySuggestionErrorKey : @"Could not load data from the save state"
        }];
    }

    return NO;
}

/*
 CrabEmu callbacks
 */

void sound_update_buffer(signed short *buf, int length)
{
    //NSLog(@"%s %p", __FUNCTION__, ringBuffer);
    [ringBuffer write:buf maxLength:length];
}

int sound_init(int channels, int region)
{
    return 0;
}

void sound_shutdown(void)
{

}

void sound_reset_buffer(void)
{

}

void gui_set_viewport(int w, int h)
{
    //NSLog(@"viewport, width: %d, height: %d", w, h);
}

void gui_set_aspect(float x, float y)
{
    //NSLog(@"set_aspect, x: %f, y: %f", x, y);
}

void gui_set_title(const char *str)
{
    //NSLog(@"set_title%s", str);
}

void gui_set_console(console_t *c)
{
    cur_console = c;
}

# pragma mark - Input

const int MasterSystemMap[] = {SMS_UP, SMS_DOWN, SMS_LEFT, SMS_RIGHT, SMS_BUTTON_1, SMS_BUTTON_2, GAMEGEAR_START};
const int ColecoVisionMap[] = {COLECOVISION_UP, COLECOVISION_DOWN, COLECOVISION_LEFT, COLECOVISION_RIGHT, COLECOVISION_L_ACTION, COLECOVISION_R_ACTION, COLECOVISION_1, COLECOVISION_2, COLECOVISION_3, COLECOVISION_4, COLECOVISION_5, COLECOVISION_6, COLECOVISION_7, COLECOVISION_8, COLECOVISION_9, COLECOVISION_0, COLECOVISION_STAR, COLECOVISION_POUND};

- (oneway void)didPushGGButton:(OEGGButton)button;
{
    sms_button_pressed(1, MasterSystemMap[button]);
}

- (oneway void)didReleaseGGButton:(OEGGButton)button;
{
    sms_button_released(1, MasterSystemMap[button]);
}

- (oneway void)didPushSMSButton:(OESMSButton)button forPlayer:(NSUInteger)player;
{
    sms_button_pressed((int)player, MasterSystemMap[button]);
}

- (oneway void)didReleaseSMSButton:(OESMSButton)button forPlayer:(NSUInteger)player;
{
    sms_button_released((int)player, MasterSystemMap[button]);
}

- (oneway void)didPushSMSStartButton;
{
    sms_button_pressed(1, GAMEGEAR_START);
}

- (oneway void)didReleaseSMSStartButton;
{
    sms_button_released(1, GAMEGEAR_START);
}

- (oneway void)didPushSMSResetButton;
{
    sms_button_pressed(1, SMS_CONSOLE_RESET);
}

- (oneway void)didReleaseSMSResetButton;
{
    sms_button_released(1, SMS_CONSOLE_RESET);
}

- (oneway void)didPushSG1000Button:(OESG1000Button)button forPlayer:(NSUInteger)player
{
    //console pause, sms_z80_nmi()
    sms_button_pressed((int)player, MasterSystemMap[button]);
}

- (oneway void)didReleaseSG1000Button:(OESG1000Button)button forPlayer:(NSUInteger)player
{
    sms_button_released((int)player, MasterSystemMap[button]);
}

- (oneway void)didPushColecoVisionButton:(OEColecoVisionButton)button forPlayer:(NSUInteger)player;
{
    coleco_button_pressed((int)player, ColecoVisionMap[button]);
}

- (oneway void)didReleaseColecoVisionButton:(OEColecoVisionButton)button forPlayer:(NSUInteger)player;
{
    coleco_button_released((int)player, ColecoVisionMap[button]);
}

#pragma mark - Cheats

- (void)setCheat:(NSString *)code setType:(NSString *)type setEnabled:(BOOL)enabled
{
    // Sanitize
    code = [code stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];

    // Remove any spaces
    code = [code stringByReplacingOccurrencesOfString:@" " withString:@""];

    // Remove address-value separator
    code = [code stringByReplacingOccurrencesOfString:@"-" withString:@""];

    if (enabled)
        [cheatList setValue:@YES forKey:code];
    else
        [cheatList removeObjectForKey:code];

    sms_cheat_reset();

    NSArray *multipleCodes = [[NSArray alloc] init];

    // Apply enabled cheats found in dictionary
    for (id key in cheatList)
    {
        if ([[cheatList valueForKey:key] isEqual:@YES])
        {
            // Handle multi-line cheats
            multipleCodes = [key componentsSeparatedByString:@"+"];
            for (NSString *singleCode in multipleCodes)
            {
                if ([singleCode length] == 8)
                {
                    // Action Replay GG/SMS format: XXXX-YYYY
                    NSString *address = [singleCode substringWithRange:NSMakeRange(0, 4)];
                    NSString *value = [singleCode substringWithRange:NSMakeRange(4, 4)];

                    // Convert AR hex to int
                    uint32_t outAddress, outValue;
                    NSScanner *scanAddress = [NSScanner scannerWithString:address];
                    NSScanner *scanValue = [NSScanner scannerWithString:value];
                    [scanAddress scanHexInt:&outAddress];
                    [scanValue scanHexInt:&outValue];

                    sms_cheat_t *arCode = (sms_cheat_t *)malloc(sizeof(sms_cheat_t));
                    memset(arCode, 0, sizeof(sms_cheat_t));
                    arCode->ar_code = (outAddress << 16) | outValue;
                    strcpy(arCode->desc, [singleCode UTF8String]);
                    arCode->enabled = 1;

                    sms_cheat_add(arCode);
                    sms_cheat_enable();
                }
            }
        }
    }
}

@end
