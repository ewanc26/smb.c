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
  /* smb.asm: AreaData / EnemyData ($e7, $e9) — the zero-page object
   * stream pointers. The streams live in C arrays in the port (see
   * data/areadata.h), so the pointers are real ones, the same shape the
   * PPU's chr pointer takes. */
  const uint8_t *area_data;
  const uint8_t *enemy_data;
} CoreState;

void state_init(CoreState *state);

/* Zero page */
#define SMBC_OBJECT_OFFSET      0x0008u /* smb.asm: ObjectOffset */
#define SMBC_FRAME_COUNTER      0x0009u /* smb.asm: FrameCounter */
#define SMBC_A_B_BUTTONS        0x000au /* smb.asm: A_B_Buttons */
#define SMBC_UP_DOWN_BUTTONS    0x000bu /* smb.asm: Up_Down_Buttons */
#define SMBC_LEFT_RIGHT_BUTTONS 0x000cu /* smb.asm: Left_Right_Buttons */
#define SMBC_PREV_A_B_BUTTONS   0x000du /* smb.asm: PreviousA_B_Buttons */
#define SMBC_GAME_ENGINE_SUBROUTINE 0x000eu /* smb.asm: GameEngineSubroutine */
#define SMBC_DESTINATION_PAGE_LOC 0x0034u /* smb.asm: DestinationPageLoc */
#define SMBC_VICTORY_WALK_CONTROL 0x0035u /* smb.asm: VictoryWalkControl */
#define SMBC_PLAYER_PAGE_LOC     0x006du /* smb.asm: Player_PageLoc */
#define SMBC_PLAYER_X_POSITION   0x0086u /* smb.asm: Player_X_Position */
#define SMBC_PLAYER_MOVING_DIR   0x0045u /* smb.asm: Player_MovingDir */
#define SMBC_PLAYER_Y_HIGH_POS   0x00b5u /* smb.asm: Player_Y_HighPos */
#define SMBC_PLAYER_Y_SPEED      0x009fu /* smb.asm: Player_Y_Speed */
#define SMBC_PLAYER_OFFSCREEN_BITS 0x03d0u /* smb.asm: Player_OffscreenBits */
#define SMBC_STOMP_TIMER         0x0791u /* smb.asm: StompTimer */
#define SMBC_INJURY_TIMER        0x079eu /* smb.asm: InjuryTimer */
#define SMBC_STOMP_CHAIN_COUNTER 0x0484u /* smb.asm: StompChainCounter */

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
#define SMBC_SPRITE_TILENUMBER   0x0201u /* smb.asm: Sprite_Tilenumber */
#define SMBC_SPRITE_ATTRIBUTES   0x0202u /* smb.asm: Sprite_Attributes */
#define SMBC_SPRITE_X_POSITION   0x0203u /* smb.asm: Sprite_X_Position */

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
#define SMBC_DEMO_ACTION         0x0717u /* smb.asm: DemoAction */
#define SMBC_DEMO_ACTION_TIMER   0x0718u /* smb.asm: DemoActionTimer */
#define SMBC_PRIMARY_MSG_COUNTER 0x0719u /* smb.asm: PrimaryMsgCounter */
#define SMBC_SCREEN_LEFT_PAGE_LOC 0x071au /* smb.asm: ScreenLeft_PageLoc */
#define SMBC_SCREEN_RIGHT_PAGE_LOC 0x071bu /* smb.asm: ScreenRight_PageLoc */
#define SMBC_AREA_PARSER_TASK_NUM 0x071fu /* smb.asm: AreaParserTaskNum */
#define SMBC_COLUMN_SETS         0x071eu /* smb.asm: ColumnSets */
#define SMBC_CURRENT_NT_ADDR_HIGH 0x0720u /* smb.asm: CurrentNTAddr_High — the
                                           * high byte is first */
#define SMBC_CURRENT_NT_ADDR_LOW  0x0721u /* smb.asm: CurrentNTAddr_Low */
#define SMBC_CURRENT_PAGE_LOC    0x0725u /* smb.asm: CurrentPageLoc */
#define SMBC_CURRENT_COLUMN_POS  0x0726u /* smb.asm: CurrentColumnPos */
#define SMBC_BLOCK_BUFFER_COLUMN_POS 0x06a0u /* smb.asm: BlockBufferColumnPos */
#define SMBC_METATILE_BUFFER     0x06a1u /* smb.asm: MetatileBuffer
                                          * ($06a1-$06ad) */
#define SMBC_BLOCK_BUFFER_1      0x0500u /* smb.asm: Block_Buffer_1 */
#define SMBC_BLOCK_BUFFER_2      0x05d0u /* smb.asm: Block_Buffer_2 */
#define SMBC_TERRAIN_CONTROL     0x0727u /* smb.asm: TerrainControl */
#define SMBC_FOREGROUND_SCENERY  0x0741u /* smb.asm: ForegroundScenery */
#define SMBC_BACKGROUND_SCENERY  0x0742u /* smb.asm: BackgroundScenery */
#define SMBC_ATTRIBUTE_BUFFER    0x03f9u /* smb.asm: AttributeBuffer
                                          * ($03f9-$03ff) */
#define SMBC_VRAM_BUFFER2        0x0341u /* smb.asm: VRAM_Buffer2 */
#define SMBC_SPRITE0_HIT_FLAG    0x0722u /* smb.asm: Sprite0HitDetectFlag */
#define SMBC_AREA_STYLE          0x0733u /* smb.asm: AreaStyle */
#define SMBC_SCREEN_ROUTINE_TASK 0x073cu /* smb.asm: ScreenRoutineTask */
#define SMBC_SCROLL_THIRTY_TWO   0x073du /* smb.asm: ScrollThirtyTwo */
#define SMBC_HORIZONTAL_SCROLL   0x073fu /* smb.asm: HorizontalScroll */
#define SMBC_BACKGROUND_COLOR_CTRL 0x0744u /* smb.asm: BackgroundColorCtrl */
#define SMBC_VERTICAL_SCROLL     0x0740u /* smb.asm: VerticalScroll */
#define SMBC_TIMER_CONTROL       0x0747u /* smb.asm: TimerControl */
#define SMBC_SECONDARY_MSG_COUNTER 0x0749u /* smb.asm: SecondaryMsgCounter */
#define SMBC_AREA_TYPE           0x074eu /* smb.asm: AreaType */
#define SMBC_AREA_POINTER        0x0750u /* smb.asm: AreaPointer */
#define SMBC_CURRENT_PLAYER      0x0753u /* smb.asm: CurrentPlayer */
#define SMBC_PLAYER_SIZE         0x0754u /* smb.asm: PlayerSize */
#define SMBC_PLAYER_STATUS       0x0756u /* smb.asm: PlayerStatus */
#define SMBC_FETCH_NEW_GAME_TIMER_FLAG 0x0757u /* smb.asm: FetchNewGameTimerFlag */
#define SMBC_ONSCREEN_PLAYER_INFO 0x075au /* smb.asm: OnscreenPlayerInfo (7 bytes) */
#define SMBC_NUMBEROF_LIVES      0x075au /* smb.asm: NumberofLives (info byte 0) */
#define SMBC_LEVEL_NUMBER        0x075cu /* smb.asm: LevelNumber */
#define SMBC_HIDDEN_1UP_FLAG     0x075du /* smb.asm: Hidden1UpFlag */
#define SMBC_WORLD_NUMBER        0x075fu /* smb.asm: WorldNumber */
#define SMBC_AREA_NUMBER         0x0760u /* smb.asm: AreaNumber */
#define SMBC_OFFSCREEN_PLAYER_INFO 0x0761u /* smb.asm: OffscreenPlayerInfo (7 bytes) */
#define SMBC_OFFSCR_NUMBEROF_LIVES 0x0761u /* smb.asm: OffScr_NumberofLives (info byte 0) */
#define SMBC_OFFSCR_HIDDEN_1UP_FLAG 0x0764u /* smb.asm: OffScr_Hidden1UpFlag */
#define SMBC_OFFSCR_WORLD_NUMBER 0x0766u /* smb.asm: OffScr_WorldNumber */
#define SMBC_OFFSCR_AREA_NUMBER  0x0767u /* smb.asm: OffScr_AreaNumber */
#define SMBC_SCROLL_FRACTIONAL   0x0768u /* smb.asm: ScrollFractional */
#define SMBC_PRIMARY_HARD_MODE   0x076au /* smb.asm: PrimaryHardMode */
#define SMBC_WORLD_SELECT_NUMBER 0x076bu /* smb.asm: WorldSelectNumber */
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
#define SMBC_SELECT_TIMER        0x0780u /* smb.asm: SelectTimer */
#define SMBC_STAR_INVINCIBLE_TIMER 0x079fu /* smb.asm: StarInvincibleTimer */
#define SMBC_SCREEN_TIMER        0x07a0u /* smb.asm: ScreenTimer */
#define SMBC_WORLD_END_TIMER     0x07a1u /* smb.asm: WorldEndTimer */
#define SMBC_DEMO_TIMER          0x07a2u /* smb.asm: DemoTimer */
#define SMBC_PSEUDO_RANDOM_BIT_REG 0x07a7u /* smb.asm: PseudoRandomBitReg */
#define SMBC_TOP_SCORE_DISPLAY   0x07d7u /* smb.asm: TopScoreDisplay (6 digits) */
#define SMBC_PLAYER_SCORE_DISPLAY 0x07ddu /* smb.asm: PlayerScoreDisplay */
#define SMBC_SCORE_AND_COIN_DISPLAY 0x07ddu /* smb.asm: ScoreAndCoinDisplay (24 bytes) */
#define SMBC_WORLD_SELECT_ENABLE_FLAG 0x07fcu /* smb.asm: WorldSelectEnableFlag */
#define SMBC_CONTINUE_WORLD      0x07fdu /* smb.asm: ContinueWorld */
#define SMBC_WARM_BOOT_VALIDATION 0x07ffu /* smb.asm: WarmBootValidation */

/* world values */
#define SMBC_WORLD_8             0x07u   /* smb.asm: World8 */

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
#define SMBC_GAME_TIMER_SETTING   0x0715u /* smb.asm: GameTimerSetting */
#define SMBC_AREA_ADDRS_L_OFFSET  0x074fu /* smb.asm: AreaAddrsLOffset */

/* area object decoder state (smb.asm: ProcessAreaData/DecodeAreaData) */
#define SMBC_SCROLL_LOCK 0x0723u         /* smb.asm: ScrollLock */
#define SMBC_BEHIND_AREA_PARSER_FLAG 0x0729u /* smb.asm: BehindAreaParserFlag */
#define SMBC_AREA_OBJECT_PAGE_LOC 0x072au /* smb.asm: AreaObjectPageLoc */
#define SMBC_AREA_OBJECT_PAGE_SEL 0x072bu /* smb.asm: AreaObjectPageSel */
#define SMBC_AREA_DATA_OFFSET 0x072cu    /* smb.asm: AreaDataOffset */
#define SMBC_AREA_OBJ_OFFSET_BUFFER 0x072du /* smb.asm: AreaObjOffsetBuffer */
#define SMBC_AREA_OBJECT_LENGTH 0x0730u  /* smb.asm: AreaObjectLength */
#define SMBC_STAIRCASE_CONTROL 0x0734u   /* smb.asm: StaircaseControl */
#define SMBC_AREA_OBJECT_HEIGHT 0x0735u  /* smb.asm: AreaObjectHeight */
#define SMBC_MUSHROOM_LEDGE_HALF_LEN 0x0736u /* smb.asm: MushroomLedgeHalfLen */
#define SMBC_LOOP_COMMAND 0x0745u        /* smb.asm: LoopCommand */
#define SMBC_HIDDEN_1UP_FLAG 0x075du     /* smb.asm: Hidden1UpFlag */
#define SMBC_WARP_ZONE_CONTROL 0x06d6u   /* smb.asm: WarpZoneControl */

/* enemy/cannon/whirlpool buffers touched by the object handlers */
#define SMBC_ENEMY_FLAG 0x000fu          /* smb.asm: Enemy_Flag */
#define SMBC_ENEMY_ID 0x0016u            /* smb.asm: Enemy_ID */
#define SMBC_ENEMY_STATE 0x001eu         /* smb.asm: Enemy_State */
#define SMBC_JUMPSPRING_FIXED_Y_POS 0x58u /* smb.asm: Jumpspring_FixedYPos */
#define SMBC_JUMPSPRING_FORCE 0x06dbu     /* smb.asm: JumpspringForce */
#define SMBC_JUMPSPRING_TIMER 0x0786u     /* smb.asm: JumpspringTimer */
#define SMBC_PIRANHA_PLANT_Y_SPEED 0x58u /* smb.asm: PiranhaPlant_Y_Speed
                                          * (shares $58 with Jumpspring) */
#define SMBC_SPROBJ_PAGE_LOC 0x006du     /* smb.asm: SprObject_PageLoc */
#define SMBC_SPROBJ_X_POSITION 0x0086u   /* smb.asm: SprObject_X_Position */
#define SMBC_SPROBJ_Y_POSITION 0x00ceu   /* smb.asm: SprObject_Y_Position */
#define SMBC_ENEMY_PAGE_LOC 0x006eu      /* smb.asm: Enemy_PageLoc */
#define SMBC_ENEMY_X_POSITION 0x0087u    /* smb.asm: Enemy_X_Position */
#define SMBC_PIRANHA_PLANT_MOVE_FLAG 0xa0u /* smb.asm: PiranhaPlant_MoveFlag */
#define SMBC_ENEMY_Y_HIGH_POS 0x00b6u    /* smb.asm: Enemy_Y_HighPos */
#define SMBC_ENEMY_Y_POSITION 0x00cfu    /* smb.asm: Enemy_Y_Position */
#define SMBC_FLAGPOLE_FNUM_Y_POS 0x010du /* smb.asm: FlagpoleFNum_Y_Pos */
#define SMBC_PIRANHA_PLANT_UP_Y_POS 0x0417u /* smb.asm: PiranhaPlantUpYPos */
#define SMBC_PIRANHA_PLANT_DOWN_Y_POS 0x0434u /* smb.asm: PiranhaPlantDownYPos */
#define SMBC_CANNON_OFFSET 0x046au       /* smb.asm: Cannon_Offset */
#define SMBC_CANNON_PAGE_LOC 0x046bu     /* smb.asm: Cannon_PageLoc */
#define SMBC_CANNON_X_POSITION 0x0471u   /* smb.asm: Cannon_X_Position */
#define SMBC_CANNON_Y_POSITION 0x0477u   /* smb.asm: Cannon_Y_Position */
#define SMBC_WHIRLPOOL_OFFSET 0x046au    /* smb.asm: Whirlpool_Offset */
#define SMBC_WHIRLPOOL_PAGE_LOC 0x046bu  /* smb.asm: Whirlpool_PageLoc */
#define SMBC_WHIRLPOOL_LEFT_EXTENT 0x0471u /* smb.asm: Whirlpool_LeftExtent */
#define SMBC_WHIRLPOOL_LENGTH 0x0477u    /* smb.asm: Whirlpool_Length */
#define SMBC_ENEMY_BOUND_BOX_CTRL 0x049au /* smb.asm: Enemy_BoundBoxCtrl */
#define SMBC_BRICK_COIN_TIMER_FLAG 0x06bcu /* smb.asm: BrickCoinTimerFlag */
#define SMBC_ENEMY_FRENZY_QUEUE 0x06cdu  /* smb.asm: EnemyFrenzyQueue */
#define SMBC_BACKLOADING_FLAG   0x0728u /* smb.asm: BackloadingFlag */
#define SMBC_CLOUD_TYPE_OVERRIDE 0x0743u /* smb.asm: CloudTypeOverride */
#define SMBC_ALT_ENTRANCE_CONTROL 0x0752u /* smb.asm: AltEntranceControl */
#define SMBC_GAME_TIMER_EXPIRED_FLAG 0x0759u /* smb.asm: GameTimerExpiredFlag */
#define SMBC_DISABLE_INTERMEDIATE 0x0769u /* smb.asm: DisableIntermediate */
#define SMBC_BAL_PLATFORM_ALIGNMENT 0x03a0u /* smb.asm: BalPlatformAlignment */

/* enemy data interpreter state (smb.asm: ProcessEnemyData region) */
#define SMBC_PLAYER_STATE 0x001du         /* smb.asm: Player_State */
#define SMBC_PLAYER_Y_POSITION 0x00ceu    /* smb.asm: Player_Y_Position */
#define SMBC_FLOATEY_NUM_CONTROL 0x0110u  /* smb.asm: FloateyNum_Control */
#define SMBC_FLOATEY_NUM_X_POS 0x0117u   /* smb.asm: FloateyNum_X_Pos */
#define SMBC_FLOATEY_NUM_Y_POS 0x011eu   /* smb.asm: FloateyNum_Y_Pos */
#define SMBC_FLOATEY_NUM_TIMER 0x012cu   /* smb.asm: FloateyNum_Timer */
#define SMBC_SFX_ENEMY_SMACK 0x08u       /* smb.asm: Sfx_EnemySmack */
#define SMBC_SHELL_CHAIN_COUNTER 0x0125u  /* smb.asm: ShellChainCounter */
#define SMBC_VINE_FLAG_OFFSET 0x0398u     /* smb.asm: VineFlagOffset */
#define SMBC_VINE_HEIGHT 0x0399u          /* smb.asm: VineHeight */
#define SMBC_ENEMY_SPR_ATTRIB 0x03c5u     /* smb.asm: Enemy_SprAttrib */
#define SMBC_ENEMY_REL_XPOS 0x03aeu      /* smb.asm: Enemy_Rel_XPos */
#define SMBC_ENEMY_REL_YPOS 0x03b9u      /* smb.asm: Enemy_Rel_YPos */
#define SMBC_ENEMY_OFFSCREEN_BITS 0x03d1u /* smb.asm: Enemy_OffscreenBits */
#define SMBC_ENEMY_OFFSCR_MASKED 0x03d8u  /* smb.asm: EnemyOffscrBitsMasked */
#define SMBC_ENEMY_COLLISION_BITS 0x0491u /* smb.asm: Enemy_CollisionBits */
#define SMBC_ENEMY_OFFSCR_BITS_MASKED 0x03d8u /* smb.asm: EnemyOffscrBitsMasked */
#define SMBC_ENEMY_FRENZY_BUFFER 0x06cbu  /* smb.asm: EnemyFrenzyBuffer */
#define SMBC_SECONDARY_HARD_MODE 0x06ccu  /* smb.asm: SecondaryHardMode */
#define SMBC_NUMBER_OF_GROUP_ENEMIES 0x06d3u /* smb.asm: NumberofGroupEnemies */
#define SMBC_MULTI_LOOP_CORRECT_CNTR 0x06d9u /* smb.asm: MultiLoopCorrectCntr */
#define SMBC_MULTI_LOOP_PASS_CNTR 0x06dau /* smb.asm: MultiLoopPassCntr */
#define SMBC_SCREEN_LEFT_X_POS 0x071cu    /* smb.asm: ScreenLeft_X_Pos */
#define SMBC_SCREEN_RIGHT_X_POS 0x071du   /* smb.asm: ScreenRight_X_Pos */
#define SMBC_ENEMY_DATA_OFFSET 0x0739u    /* smb.asm: EnemyDataOffset */
#define SMBC_ENEMY_OBJECT_PAGE_LOC 0x073au /* smb.asm: EnemyObjectPageLoc */
#define SMBC_ENEMY_OBJECT_PAGE_SEL 0x073bu /* smb.asm: EnemyObjectPageSel */
#define SMBC_ENTRANCE_PAGE 0x0751u        /* smb.asm: EntrancePage */
#define SMBC_ENEMY_FRAME_TIMER 0x078au    /* smb.asm: EnemyFrameTimer */
#define SMBC_ENEMY_INTERVAL_TIMER 0x0796u /* smb.asm: EnemyIntervalTimer */

/* enemy init state (smb.asm: InitEnemyRoutines region). Several of
 * these deliberately share addresses — the asm reuses enemy buffer
 * locations per object type */
#define SMBC_POWER_UP_TYPE 0x0039u        /* smb.asm: PowerUpType */
#define SMBC_ENEMY_MOVING_DIR 0x0046u     /* smb.asm: Enemy_MovingDir */
#define SMBC_X_MOVE_PRIMARY_COUNTER 0x00a0u /* smb.asm: XMovePrimaryCounter */
#define SMBC_ENEMY_X_SPEED 0x0058u        /* smb.asm: Enemy_X_Speed (shared) */
#define SMBC_ENEMY_Y_SPEED 0x00a0u        /* smb.asm: Enemy_Y_Speed (shared) */
#define SMBC_BLOOPER_MOVE_SPEED 0x0058u   /* smb.asm: BlooperMoveSpeed */
#define SMBC_RED_PTROOPA_CENTER_Y_POS 0x0058u /* smb.asm: RedPTroopaCenterYPos */
#define SMBC_CHEEP_CHEEP_MOVE_M_FLAG 0x0058u /* smb.asm: CheepCheepMoveMFlag */
#define SMBC_X_MOVE_SECONDARY_COUNTER 0x0058u /* smb.asm: XMoveSecondaryCounter */
#define SMBC_FIREBAR_SPIN_STATE_LOW 0x0058u /* smb.asm: FirebarSpinState_Low */
#define SMBC_Y_PLATFORM_CENTER_Y_POS 0x0058u /* smb.asm: YPlatformCenterYPos */
#define SMBC_BLOCK_PAGE_LOC 0x0076u       /* smb.asm: Block_PageLoc */
#define SMBC_BLOCK_X_POSITION 0x008fu     /* smb.asm: Block_X_Position */
#define SMBC_BLOCK_Y_POSITION 0x00d7u     /* smb.asm: Block_Y_Position */
#define SMBC_FIREBAR_SPIN_DIRECTION 0x0034u /* smb.asm: FirebarSpinDirection */
#define SMBC_BOWSER_BODY_CONTROLS 0x0363u /* smb.asm: BowserBodyControls */
#define SMBC_BOWSER_GFX_FLAG  0x036au     /* smb.asm: BowserGfxFlag */
#define SMBC_VERTICAL_FLIP_FLAG 0x0109u   /* smb.asm: VerticalFlipFlag */
#define SMBC_JUMPSPRING_ANIM_CTRL 0x070eu /* smb.asm: JumpspringAnimCtrl */
#define SMBC_ENEMY_SPR_DATA_OFFSET 0x06e5u /* smb.asm: Enemy_SprDataOffset */
#define SMBC_PIRANHA_PLANT_Y_SPEED 0x58u /* smb.asm: PiranhaPlant_Y_Speed */
#define SMBC_BOWSER_FEET_COUNTER 0x0364u  /* smb.asm: BowserFeetCounter */
#define SMBC_BOWSER_MOVEMENT_SPEED 0x0365u /* smb.asm: BowserMovementSpeed */
#define SMBC_BOWSER_ORIG_X_POS 0x0366u    /* smb.asm: BowserOrigXPos */
#define SMBC_BOWSER_FRONT_OFFSET 0x0368u  /* smb.asm: BowserFront_Offset */
#define SMBC_BRIDGE_COLLAPSE_OFFSET 0x0369u /* smb.asm: BridgeCollapseOffset */
#define SMBC_FIREBAR_SPIN_SPEED 0x0388u   /* smb.asm: FirebarSpinSpeed */
#define SMBC_VINE_OBJ_OFFSET 0x039au      /* smb.asm: VineObjOffset */
#define SMBC_VINE_START_Y_POSITION 0x039du /* smb.asm: VineStart_Y_Position */
#define SMBC_PLATFORM_COLLISION_FLAG 0x03a2u /* smb.asm: PlatformCollisionFlag */
#define SMBC_HAMMER_THROWING_TIMER 0x03a2u /* smb.asm: HammerThrowingTimer */
#define SMBC_SPROBJ_REL_XPOS 0x03adu       /* smb.asm: SprObject_Rel_XPos */
#define SMBC_SPROBJ_REL_YPOS 0x03b8u       /* smb.asm: SprObject_Rel_YPos */
#define SMBC_SPROBJ_BOUND_BOX_CTRL 0x0499u /* smb.asm: SprObj_BoundBoxCtrl */
#define SMBC_BOUNDING_BOX_UL_CORNER 0x04acu /* smb.asm: BoundingBox_UL_Corner */
#define SMBC_BOUNDING_BOX_LR_CORNER 0x04aeu /* smb.asm: BoundingBox_LR_Corner */
#define SMBC_BOUNDING_BOX_UL_XPOS 0x04acu  /* smb.asm: BoundingBox_UL_XPos */
#define SMBC_BOUNDING_BOX_DR_XPOS 0x04aeu  /* smb.asm: BoundingBox_DR_XPos */
#define SMBC_ENEMY_BBOX_COORD 0x04b0u      /* smb.asm: EnemyBoundingBoxCoord */
#define SMBC_HAMMER_BRO_JUMP_TIMER 0x003cu   /* smb.asm: HammerBroJumpTimer */
#define SMBC_MISC_STATE 0x002au              /* smb.asm: Misc_State */
#define SMBC_MISC_BOUND_BOX_CTRL 0x04a2u     /* smb.asm: Misc_BoundBoxCtrl */
#define SMBC_ENEMY_BOUND_BOX_CTRL 0x049au    /* smb.asm: Enemy_BoundBoxCtrl */
#define SMBC_HAMMER_ENEMY_OFFSET 0x06aeu     /* smb.asm: HammerEnemyOffset */
#define SMBC_ENEMY_X_MOVE_FORCE 0x0401u   /* smb.asm: Enemy_X_MoveForce (shared) */
#define SMBC_RED_PTROOPA_ORIG_X_POS 0x0401u /* smb.asm: RedPTroopaOrigXPos */
#define SMBC_Y_PLATFORM_TOP_Y_POS 0x0401u /* smb.asm: YPlatformTopYPos */
#define SMBC_ENEMY_Y_MOVE_FORCE 0x0434u   /* smb.asm: Enemy_Y_MoveForce (shared) */
#define SMBC_CHEEP_CHEEP_ORIG_Y_POS 0x0434u /* smb.asm: CheepCheepOrigYPos */
#define SMBC_BOWSER_HIT_POINTS 0x0483u    /* smb.asm: BowserHitPoints */
#define SMBC_DUPLICATE_OBJ_OFFSET 0x06cfu /* smb.asm: DuplicateObj_Offset */
#define SMBC_LAKITU_REAPPEAR_TIMER 0x06d1u /* smb.asm: LakituReappearTimer */
#define SMBC_BOWSER_FIRE_BREATH_TIMER 0x0790u /* smb.asm: BowserFireBreathTimer */
#define SMBC_PLAYER_X_SPEED 0x0057u       /* smb.asm: Player_X_Speed */
#define SMBC_LAKITU_MOVE_SPEED 0x0058u    /* smb.asm: LakituMoveSpeed (shared) */
#define SMBC_EXPLOSION_GFX_COUNTER 0x0058u /* smb.asm: ExplosionGfxCounter (shared) */
#define SMBC_FIREBALL_REL_XPOS 0x03afu    /* smb.asm: Fireball_Rel_XPos (shared) */
#define SMBC_FIREBALL_REL_YPOS 0x03bau    /* smb.asm: Fireball_Rel_YPos (shared) */
#define SMBC_LAKITU_MOVE_DIRECTION 0x00a0u /* smb.asm: LakituMoveDirection (shared) */
#define SMBC_EXPLOSION_TIMER_COUNTER 0x00a0u /* smb.asm: ExplosionTimerCounter (shared) */
#define SMBC_BOWSER_FLAME_TIMER_CTRL 0x0367u /* smb.asm: BowserFlameTimerCtrl */
#define SMBC_ENEMY_YMF_DUMMY 0x0417u      /* smb.asm: Enemy_YMF_Dummy (shared) */
#define SMBC_BOWSER_FLAME_PRANDOM_OFS 0x0417u /* smb.asm: BowserFlamePRandomOfs */
#define SMBC_FIREWORKS_COUNTER 0x06d7u    /* smb.asm: FireworksCounter */
#define SMBC_STAR_FLAG_TASK_CONTROL 0x0746u /* smb.asm: StarFlagTaskControl */
#define SMBC_GAME_TIMER_DISPLAY 0x07f8u     /* smb.asm: GameTimerDisplay */
#define SMBC_BITM_FILTER 0x06ddu          /* smb.asm: BitMFilter */
#define SMBC_SCROLL_AMOUNT 0x0775u        /* smb.asm: ScrollAmount */
#define SMBC_FRENZY_ENEMY_TIMER 0x078fu   /* smb.asm: FrenzyEnemyTimer */
#define SMBC_AREA_MUSIC_QUEUE   0x00fbu /* smb.asm: AreaMusicQueue */

/* zero page scratch used by the screen update path */
#define SMBC_TEMP_VRAM_PTR 0x0000u /* smb.asm: $00/$01, the ($00),y indirect */

/* zero page scratch used by the block buffer path (smb.asm:
 * BlockBufferCollision's $02-$07 outputs) */
#define SMBC_BB_Y_COORD 0x0002u   /* block row offset into the buffer */
#define SMBC_BB_METATILE 0x0003u  /* metatile found */
#define SMBC_BB_LOW_NYBBLE 0x0004u /* low nybble of the x or y coord */
#define SMBC_BB_X_COORD 0x0005u   /* x coordinate plus adder */
#define SMBC_BB_PTR_LO 0x0006u    /* block buffer pointer, low */
#define SMBC_BB_PTR_HI 0x0007u    /* block buffer pointer, high */
#define SMBC_SIDE_CHECK_DIR 0x00ebu /* smb.asm: $eb, DoEnemySideCheck's counter */

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
#define SMBC_SFX_POWER_UP_GRAB     0x20u /* smb.asm: Sfx_PowerUpGrab */
#define SMBC_STAR_POWER_MUSIC      0x40u /* smb.asm: StarPowerMusic */
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
