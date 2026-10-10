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
#define SMBC_GAME_ENGINE_SUBROUTINE 0x000eu /* smb.asm: GameEngineSubroutine */

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
#define SMBC_SCREEN_ROUTINE_TASK 0x073cu /* smb.asm: ScreenRoutineTask */
#define SMBC_HORIZONTAL_SCROLL   0x073fu /* smb.asm: HorizontalScroll */
#define SMBC_VERTICAL_SCROLL     0x0740u /* smb.asm: VerticalScroll */
#define SMBC_TIMER_CONTROL       0x0747u /* smb.asm: TimerControl */
#define SMBC_AREA_TYPE           0x074eu /* smb.asm: AreaType */
#define SMBC_AREA_POINTER        0x0750u /* smb.asm: AreaPointer */
#define SMBC_CURRENT_PLAYER      0x0753u /* smb.asm: CurrentPlayer */
#define SMBC_PLAYER_SIZE         0x0754u /* smb.asm: PlayerSize */
#define SMBC_PLAYER_STATUS       0x0756u /* smb.asm: PlayerStatus */
#define SMBC_FETCH_NEW_GAME_TIMER_FLAG 0x0757u /* smb.asm: FetchNewGameTimerFlag */
#define SMBC_ONSCREEN_PLAYER_INFO 0x075au /* smb.asm: OnscreenPlayerInfo (7 bytes) */
#define SMBC_WORLD_NUMBER        0x075fu /* smb.asm: WorldNumber */
#define SMBC_AREA_NUMBER         0x0760u /* smb.asm: AreaNumber */
#define SMBC_OFFSCREEN_PLAYER_INFO 0x0761u /* smb.asm: OffscreenPlayerInfo (7 bytes) */
#define SMBC_OFFSCR_NUMBEROF_LIVES 0x0761u /* smb.asm: OffScr_NumberofLives (info byte 0) */
#define SMBC_OFFSCR_WORLD_NUMBER 0x0766u /* smb.asm: OffScr_WorldNumber */
#define SMBC_OFFSCR_AREA_NUMBER  0x0767u /* smb.asm: OffScr_AreaNumber */
#define SMBC_OPER_MODE           0x0770u /* smb.asm: OperMode */
#define SMBC_OPER_MODE_TASK      0x0772u /* smb.asm: OperMode_Task */
#define SMBC_VRAM_BUFFER_ADDR_CTRL 0x0773u /* smb.asm: VRAM_Buffer_AddrCtrl */
#define SMBC_DISABLE_SCREEN_FLAG 0x0774u /* smb.asm: DisableScreenFlag */
#define SMBC_GAME_PAUSE_STATUS   0x0776u /* smb.asm: GamePauseStatus */
#define SMBC_GAME_PAUSE_TIMER    0x0777u /* smb.asm: GamePauseTimer */
#define SMBC_MIRROR_PPU_CTRL1    0x0778u /* smb.asm: Mirror_PPU_CTRL_REG1 */
#define SMBC_MIRROR_PPU_CTRL2    0x0779u /* smb.asm: Mirror_PPU_CTRL_REG2 */
#define SMBC_NUMBER_OF_PLAYERS   0x077au /* smb.asm: NumberOfPlayers */
#define SMBC_INTERVAL_TIMER_CONTROL 0x077fu /* smb.asm: IntervalTimerControl */
#define SMBC_TIMERS              0x0780u /* smb.asm: Timers ($0780-$07a3) */
#define SMBC_SCREEN_TIMER        0x07a0u /* smb.asm: ScreenTimer */
#define SMBC_PSEUDO_RANDOM_BIT_REG 0x07a7u /* smb.asm: PseudoRandomBitReg */
#define SMBC_TOP_SCORE_DISPLAY   0x07d7u /* smb.asm: TopScoreDisplay (6 digits) */
#define SMBC_PLAYER_SCORE_DISPLAY 0x07ddu /* smb.asm: PlayerScoreDisplay */
#define SMBC_CONTINUE_WORLD      0x07fdu /* smb.asm: ContinueWorld */
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

/* display digits and score arithmetic */
#define SMBC_DIGIT_MODIFIER  0x0134u /* smb.asm: DigitModifier ($0134-$0139) */
#define SMBC_DISPLAY_DIGITS  0x07d7u /* smb.asm: DisplayDigits — same base as
                                      * TopScoreDisplay, used generically */

/* game setup state */
#define SMBC_PLAYER_ENTRANCE_CTRL 0x0710u /* smb.asm: PlayerEntranceCtrl */
#define SMBC_SCREEN_LEFT_PAGE_LOC 0x071au /* smb.asm: ScreenLeft_PageLoc */
#define SMBC_BACKLOADING_FLAG   0x0728u /* smb.asm: BackloadingFlag */
#define SMBC_CLOUD_TYPE_OVERRIDE 0x0743u /* smb.asm: CloudTypeOverride */
#define SMBC_ALT_ENTRANCE_CONTROL 0x0752u /* smb.asm: AltEntranceControl */
#define SMBC_GAME_TIMER_EXPIRED_FLAG 0x0759u /* smb.asm: GameTimerExpiredFlag */
#define SMBC_DISABLE_INTERMEDIATE 0x0769u /* smb.asm: DisableIntermediate */
#define SMBC_BAL_PLATFORM_ALIGNMENT 0x03a0u /* smb.asm: BalPlatformAlignment */
#define SMBC_AREA_MUSIC_QUEUE   0x00fbu /* smb.asm: AreaMusicQueue */

/* zero page scratch used by the screen update path */
#define SMBC_TEMP_VRAM_PTR 0x0000u /* smb.asm: $00/$01, the ($00),y indirect */

/* boot entry offsets, low bytes into the RAM page (smb.asm: constants) */
#define SMBC_WARM_BOOT_OFFSET 0xd6u /* smb.asm: WarmBootOffset = <$07d6 */
#define SMBC_COLD_BOOT_OFFSET 0xfeu /* smb.asm: ColdBootOffset = <$07fe */

/* sound engine state (smb.asm: sound engine RAM defines, lines 506-549) */
#define SMBC_NOTE_LEN_LOOKUP_TBL_OFS 0x00f0u /* smb.asm: NoteLenLookupTblOfs */
#define SMBC_SQUARE1_SOUND_BUFFER 0x00f1u /* smb.asm: Square1SoundBuffer */
#define SMBC_SQUARE2_SOUND_BUFFER 0x00f2u /* smb.asm: Square2SoundBuffer */
#define SMBC_NOISE_SOUND_BUFFER  0x00f3u /* smb.asm: NoiseSoundBuffer */
#define SMBC_AREA_MUSIC_BUFFER   0x00f4u /* smb.asm: AreaMusicBuffer */
#define SMBC_MUSIC_DATA_LOW      0x00f5u /* smb.asm: MusicDataLow */
#define SMBC_MUSIC_DATA_HIGH     0x00f6u /* smb.asm: MusicDataHigh */
#define SMBC_MUSIC_OFFSET_SQUARE2 0x00f7u /* smb.asm: MusicOffset_Square2 */
#define SMBC_MUSIC_OFFSET_SQUARE1 0x00f8u /* smb.asm: MusicOffset_Square1 */
#define SMBC_MUSIC_OFFSET_TRIANGLE 0x00f9u /* smb.asm: MusicOffset_Triangle */
#define SMBC_EVENT_MUSIC_QUEUE   0x00fcu /* smb.asm: EventMusicQueue */
#define SMBC_NOISE_SOUND_QUEUE   0x00fdu /* smb.asm: NoiseSoundQueue */
#define SMBC_SQUARE2_SOUND_QUEUE 0x00feu /* smb.asm: Square2SoundQueue */
#define SMBC_SQUARE1_SOUND_QUEUE 0x00ffu /* smb.asm: Square1SoundQueue */
#define SMBC_MUSIC_OFFSET_NOISE  0x07b0u /* smb.asm: MusicOffset_Noise */
#define SMBC_EVENT_MUSIC_BUFFER  0x07b1u /* smb.asm: EventMusicBuffer */
#define SMBC_PAUSE_SOUND_BUFFER  0x07b2u /* smb.asm: PauseSoundBuffer */
#define SMBC_SQU2_NOTE_LEN_BUFFER 0x07b3u /* smb.asm: Squ2_NoteLenBuffer */
#define SMBC_SQU2_NOTE_LEN_COUNTER 0x07b4u /* smb.asm: Squ2_NoteLenCounter */
#define SMBC_SQU2_ENVELOPE_DATA_CTRL 0x07b5u /* smb.asm: Squ2_EnvelopeDataCtrl */
#define SMBC_SQU1_NOTE_LEN_COUNTER 0x07b6u /* smb.asm: Squ1_NoteLenCounter */
#define SMBC_SQU1_ENVELOPE_DATA_CTRL 0x07b7u /* smb.asm: Squ1_EnvelopeDataCtrl */
#define SMBC_TRI_NOTE_LEN_BUFFER 0x07b8u /* smb.asm: Tri_NoteLenBuffer */
#define SMBC_TRI_NOTE_LEN_COUNTER 0x07b9u /* smb.asm: Tri_NoteLenCounter */
#define SMBC_NOISE_BEAT_LEN_COUNTER 0x07bau /* smb.asm: Noise_BeatLenCounter */
#define SMBC_SQU1_SFX_LEN_COUNTER 0x07bbu /* smb.asm: Squ1_SfxLenCounter */
#define SMBC_SQU2_SFX_LEN_COUNTER 0x07bdu /* smb.asm: Squ2_SfxLenCounter */
#define SMBC_SFX_SECONDARY_COUNTER 0x07beu /* smb.asm: Sfx_SecondaryCounter */
#define SMBC_NOISE_SFX_LEN_COUNTER 0x07bfu /* smb.asm: Noise_SfxLenCounter */
#define SMBC_DAC_COUNTER         0x07c0u /* smb.asm: DAC_Counter */
#define SMBC_NOISE_DATA_LOOPBACK_OFS 0x07c1u /* smb.asm: NoiseDataLoopbackOfs */
#define SMBC_NOTE_LENGTH_TBL_ADDER 0x07c4u /* smb.asm: NoteLengthTblAdder */
#define SMBC_AREA_MUSIC_BUFFER_ALT 0x07c5u /* smb.asm: AreaMusicBuffer_Alt */
#define SMBC_PAUSE_MODE_FLAG     0x07c6u /* smb.asm: PauseModeFlag */
#define SMBC_GROUND_MUSIC_HEADER_OFS 0x07c7u /* smb.asm: GroundMusicHeaderOfs */
#define SMBC_ALT_REG_CONTENT_FLAG 0x07cau /* smb.asm: AltRegContentFlag */

/* ---- sound effect queue bits (smb.asm: Sfx_*) ----------------------------
 * Square 1 and the noise channel each share one bit layout; square 2 has
 * its own. */
#define SMBC_SFX_SMALL_JUMP        0x80u /* smb.asm: Sfx_SmallJump */
#define SMBC_SFX_BIG_JUMP          0x01u /* smb.asm: Sfx_BigJump */
#define SMBC_SFX_BUMP              0x02u /* smb.asm: Sfx_Bump */
#define SMBC_SFX_ENEMY_STOMP       0x04u /* smb.asm: Sfx_EnemyStomp (also swim) */
#define SMBC_SFX_ENEMY_SMACK       0x08u /* smb.asm: Sfx_EnemySmack */
#define SMBC_SFX_PIPE_DOWN_INJURY  0x10u /* smb.asm: Sfx_PipeDownInjury */
#define SMBC_SFX_FIREBALL          0x20u /* smb.asm: Sfx_Fireball */
#define SMBC_SFX_FLAGPOLE          0x40u /* smb.asm: Sfx_Flagpole */

#define SMBC_SFX_BOWSER_FALL       0x80u /* smb.asm: Sfx_BowserFall */
#define SMBC_SFX_COIN_GRAB         0x01u /* smb.asm: Sfx_CoinGrab */
#define SMBC_SFX_GROW_POWER_UP     0x02u /* smb.asm: Sfx_GrowPowerUp */
#define SMBC_SFX_GROW_VINE         0x04u /* smb.asm: Sfx_GrowVine */
#define SMBC_SFX_BLAST             0x08u /* smb.asm: Sfx_Blast */
#define SMBC_SFX_TIMER_TICK        0x10u /* smb.asm: Sfx_TimerTick */
#define SMBC_SFX_POWER_UP_GRAB     0x20u /* smb.asm: Sfx_PowerUpGrab */
#define SMBC_SFX_EXTRA_LIFE        0x40u /* smb.asm: Sfx_ExtraLife */

#define SMBC_SFX_BRICK_SHATTER     0x01u /* smb.asm: Sfx_BrickShatter */
#define SMBC_SFX_BOWSER_FLAME      0x02u /* smb.asm: Sfx_BowserFlame */
