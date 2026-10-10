/* core/state.h — the NES RAM map.
 *
 * Owns: a single flat 2KB image of the console's internal RAM
 * ($0000-$07FF), plus the named offsets the game code uses. The original
 * treats this memory as shared, fixed-address state; the port keeps that
 * shape instead of inventing structures the disassembly does not have.
 *
 * Offsets below are smb.asm's game defines, cited per line. Add further
 * names as their routines are translated — never rename one to something
 * that does not appear in the disassembly.
 */
#ifndef SMBC_CORE_STATE_H
#define SMBC_CORE_STATE_H

#include <stdint.h>

#define SMBC_RAM_SIZE 0x0800u

typedef struct {
  uint8_t ram[SMBC_RAM_SIZE];
} CoreState;

void state_init(CoreState *state);

/* Zero page */
#define SMBC_OBJECT_OFFSET      0x0008u /* smb.asm: ObjectOffset */
#define SMBC_FRAME_COUNTER      0x0009u /* smb.asm: FrameCounter */
#define SMBC_A_B_BUTTONS        0x000au /* smb.asm: A_B_Buttons */
#define SMBC_UP_DOWN_BUTTONS    0x000bu /* smb.asm: Up_Down_Buttons */
#define SMBC_PREV_A_B_BUTTONS   0x000du /* smb.asm: PreviousA_B_Buttons */

/* $0700 page */
#define SMBC_SAVED_JOYPAD1_BITS 0x06fcu /* smb.asm: SavedJoypad1Bits */
#define SMBC_SAVED_JOYPAD2_BITS 0x06fdu /* smb.asm: SavedJoypad2Bits */
#define SMBC_JOYPAD_BIT_MASK    0x074au /* smb.asm: JoypadBitMask */
#define SMBC_JOYPAD_OVERRIDE    0x0758u /* smb.asm: JoypadOverride */

#endif /* SMBC_CORE_STATE_H */

/* --- further RAM offsets, cited per line as their routines are translated */

/* zero page */
#define SMBC_PAUSE_SOUND_QUEUE   0x00fau /* smb.asm: PauseSoundQueue */

/* OAM (sprite data page) */
#define SMBC_SPRITE_DATA         0x0200u /* smb.asm: Sprite_Data */
#define SMBC_SPRITE_Y_POSITION   0x0200u /* smb.asm: Sprite_Y_Position */

/* VRAM buffers */
#define SMBC_VRAM_BUFFER1_OFFSET 0x0300u /* smb.asm: VRAM_Buffer1_Offset */
#define SMBC_VRAM_BUFFER1        0x0301u /* smb.asm: VRAM_Buffer1 */
#define SMBC_VRAM_BUFFER2_OFFSET 0x0340u /* smb.asm: VRAM_Buffer2_Offset */
#define SMBC_SPDATA_OFFSET_CTRL  0x03eeu /* smb.asm: SprDataOffset_Ctrl */

/* sprite shuffle state */
#define SMBC_SPR_SHUFFLE_AMT_OFFSET 0x06e0u /* smb.asm: SprShuffleAmtOffset */
#define SMBC_SPR_SHUFFLE_AMT     0x06e1u /* smb.asm: SprShuffleAmt (RAM, 3 bytes) */
#define SMBC_SPR_DATA_OFFSET     0x06e4u /* smb.asm: SprDataOffset (15 bytes) */
#define SMBC_ALT_SPR_DATA_OFFSET 0x06ecu /* smb.asm: Alt_SprDataOffset */
#define SMBC_MISC_SPR_DATA_OFFSET 0x06f3u /* smb.asm: Misc_SprDataOffset */

/* screen and mode state */
#define SMBC_SPRITE0_HIT_FLAG    0x0722u /* smb.asm: Sprite0HitDetectFlag */
#define SMBC_HORIZONTAL_SCROLL   0x073fu /* smb.asm: HorizontalScroll */
#define SMBC_VERTICAL_SCROLL     0x0740u /* smb.asm: VerticalScroll */
#define SMBC_TIMER_CONTROL       0x0747u /* smb.asm: TimerControl */
#define SMBC_AREA_TYPE           0x074eu /* smb.asm: AreaType */
#define SMBC_OPER_MODE           0x0770u /* smb.asm: OperMode */
#define SMBC_OPER_MODE_TASK      0x0772u /* smb.asm: OperMode_Task */
#define SMBC_VRAM_BUFFER_ADDR_CTRL 0x0773u /* smb.asm: VRAM_Buffer_AddrCtrl */
#define SMBC_DISABLE_SCREEN_FLAG 0x0774u /* smb.asm: DisableScreenFlag */
#define SMBC_GAME_PAUSE_STATUS   0x0776u /* smb.asm: GamePauseStatus */
#define SMBC_GAME_PAUSE_TIMER    0x0777u /* smb.asm: GamePauseTimer */
#define SMBC_MIRROR_PPU_CTRL1    0x0778u /* smb.asm: Mirror_PPU_CTRL_REG1 */
#define SMBC_MIRROR_PPU_CTRL2    0x0779u /* smb.asm: Mirror_PPU_CTRL_REG2 */
#define SMBC_INTERVAL_TIMER_CONTROL 0x077fu /* smb.asm: IntervalTimerControl */
#define SMBC_TIMERS              0x0780u /* smb.asm: Timers ($0780-$07a3) */
#define SMBC_PSEUDO_RANDOM_BIT_REG 0x07a7u /* smb.asm: PseudoRandomBitReg */
#define SMBC_TOP_SCORE_DISPLAY   0x07d7u /* smb.asm: TopScoreDisplay (6 digits) */
#define SMBC_PLAYER_SCORE_DISPLAY 0x07ddu /* smb.asm: PlayerScoreDisplay */
#define SMBC_WARM_BOOT_VALIDATION 0x07ffu /* smb.asm: WarmBootValidation */

/* controller bits in the game's saved-bit layout (smb.asm: A_Button etc.).
 * The serial read accumulates A first into d7, so this differs from the
 * host-side SmbcButton order in hw/joypad.h. */
#define SMBC_PAD_A      0x80u /* smb.asm: A_Button */
#define SMBC_PAD_B      0x40u /* smb.asm: B_Button */
#define SMBC_PAD_SELECT 0x20u /* smb.asm: Select_Button */
#define SMBC_PAD_START  0x10u /* smb.asm: Start_Button */
#define SMBC_PAD_UP     0x08u /* smb.asm: Up_Dir */
#define SMBC_PAD_DOWN   0x04u /* smb.asm: Down_Dir */
#define SMBC_PAD_LEFT   0x02u /* smb.asm: Left_Dir */
#define SMBC_PAD_RIGHT  0x01u /* smb.asm: Right_Dir */

/* modes of operation (smb.asm: TitleScreenModeValue etc.) */
#define SMBC_MODE_TITLE    0u
#define SMBC_MODE_GAME     1u
#define SMBC_MODE_VICTORY  2u
#define SMBC_MODE_GAME_OVER 3u

/* zero page scratch used by the screen update path */
#define SMBC_TEMP_VRAM_PTR 0x0000u /* smb.asm: $00/$01, the ($00),y indirect */

/* boot entry offsets, low bytes into the RAM page (smb.asm: constants) */
#define SMBC_WARM_BOOT_OFFSET 0xd6u /* smb.asm: WarmBootOffset = <$07d6 */
#define SMBC_COLD_BOOT_OFFSET 0xfeu /* smb.asm: ColdBootOffset = <$07fe */
