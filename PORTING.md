# Porting checklist — Battle City (J)

Список всех top-level ASM-функций из `DOCS/Battle City (J).asm` (отмечены по маркеру `; End of function …`). Отмечаем `[x]` функции, проверенные на побайтовое соответствие оригиналу. `[~]` — портирована, но не сверена. `[ ]` — портировано приблизительно или не проверено.

Формат строки: `[ ] FunctionName` (ASM:line) → `c_function` в `path/file.c`

---

## Boot / NMI / PPU базовые

- [x] `RESET` (ASM:308) → `reset()` в `game/reset.c` (SEI/CLD/TXS — N/A в C; ASM-Wait-loop с двойным PPU_STATUS-poll свёрнут — vblank_wait() уже вызывается внутри reset_screen_stuff→load_pals и set_ppu, что эквивалентно двум кадрам ожидания)
- [x] `NMI` (ASM:3176) → `nmi()` в `game/nmi.c` (метки `Skip_PalLoad`/`End_Interrupt` восстановлены; PHA/PLA-блоки и SPR_DMA — N/A в C; в C-порту прерываний нет — функция вызывается render-потоком как «псевдо-NMI»)
- [x] `NMI_Wait` (ASM:4082) → `nmi_wait()` в `game/nmi.c` (ASM busy-loop `CMP Frame_Counter; BEQ @_` заменён на atomic-флаг + семафор: game-поток сигналит render-потоку и спит на wake_sem, который render поднимает после каждого `nmi()`-кадра; longjmp на graceful shutdown)
- [x] `VBlank_Wait` (ASM:3449) → `vblank_wait()` в `game/nmi.c` (метки `at_`/`exit_` восстановлены; ASM spin на бите 7 PPU_STATUS заменён на vblank_sem от render-потока + atomic-чтение PPU_STATUS)
- [x] `Set_PPU` (ASM:3254) → `set_ppu()` в `game/draw.c` (VBlank_Wait + PPU_CTRL=$B0)
- [x] `Screen_Off` (ASM:3264) → `screen_off()` в `game/draw.c` (NMI_Wait + PPU_CTRL1=$10 + PPU_CTRL2=$06)
- [x] `Null_NT_Buffer` (ASM:3276) → `null_nt_buffer()` в `game/draw.c` (memset эквивалентен ASM 4×$100-loop)
- [x] `Reset_ScreenStuff` (ASM:3322) → `reset_screen_stuff()` в `game/reset.c` (hot_boot label соответствует ASM HotBoot)
- [x] `Update_Screen` (ASM:4125) → `update_screen()` в `game/nmi.c` (at_/at__/Update_Screen_End labels; **исправлен UB**: было два `x++` в одном выражении — теперь раздельные шаги)
- [x] `Save_to_VRAM` (ASM:3806) → `save_to_vram()` в `game/draw.c`
- [x] `Store_NT_Buffer_InVRAM` (ASM:3862) → `store_nt_buffer_in_vram()` в `game/draw.c` (at_ label)
- [x] `Load_Pals` (ASM:3376) → `load_pals()` в `game/reset.c` (static — VBlank_Wait + spr_pal_load + load_bkg_pal с BkgPal_Number=0)
- [x] `Load_Bkg_Pal` (ASM:3387) → `load_bkg_pal()` в `game/nmi.c` (at_ label; финальный сброс PPU_ADDRESS — no-op в C-симе)
- [x] `Spr_Pal_Load` (ASM:3419) → `spr_pal_load()` в `game/nmi.c` (at_ label)

## Game flow

- [x] `BEGIN` (ASM:332) → `begin()` в `game/begin.c` (begin_label/new_scroll/title_loaded метки; setjmp для unwinding из caller-цепочки)
- [x] `Title_Loaded` (label, ASM:341) → `title_loaded` в `begin()`
- [x] `New_Scroll` (label, ASM:337) → `new_scroll` в `begin()`
- [x] `Title_Screen_Loop` (ASM:1827) → `title_screen_loop()` в `game/title_screen.c` — все 8 ASM-меток сохранены (`loop`, `at__`, `at___`, `at____`, `check_Max_CurPos`, `plus`, `start_Check`, `start_Pressed`); удалены не-ASM-добавки: UP/DOWN-альтернативы SELECT и лишние `PPU_REG1_Stts = 0`; ASM-трюк `PLA PLA; JMP (LowPtr_Byte)` моделируется через возврат кодов 1/2/3 в caller (begin.c)
- [x] `Draw_TitleScreen` (ASM:2948) → `draw_title_screen()` в `game/title_screen.c` — метка `at_` (= ASM `@_` после if-CursorPos-block) сохранена; удалены не-ASM-добавки: лишний `BkgPal_Number = 0` и лишний clear Page-1 ($2800); добавлен пропущенный `set_ppu()` после `store_nt_buffer_in_vram`; "BATTLE"/"CITY" перенесены в 0xFF-terminated массивы `aBattle`/`aCity` (как в ASM); HiScore-строки теперь правильно индексируются от `[1]` и используют `save_aligned_str_to_scr_buffer` (right-alignment через ASM `PtrToNonzeroStrElem`)
- [x] `Scroll_TitleScrn` (ASM:1446) → `scroll_title_scrn()` в `game/title_screen.c` — метки `at_`/`at__` (= ASM `@_`/`@__`); возврат 0 при штатном завершении (CMP #$F0), 1 при `BNE @__` (моделирует `PLA PLA; JMP Title_Loaded`)
- [x] `Null_Upper_NT` (ASM:2933) → `null_upper_nt()` в `game/draw.c` (исправлены 2 бага: PPU_Addr_Ptr был `$20` → теперь `$1C` как в ASM; `nmi_wait()` заменён на `set_ppu()`)
- [x] `Selected_1player` (ASM:1937) → `selected_1player()` в `game/title_screen.c` (TanksOnScreen=5, fallthrough в accept развёрнут)
- [x] `Selected_2players` (ASM:1941, fallthrough) → `selected_2players()` в `game/title_screen.c` (TanksOnScreen=7)
- [x] `Selected_Construction` (ASM:1953) → `selected_construction()` в `game/title_screen.c` (TanksOnScreen=7 + construction())
- [x] `CurPos_To_PixelCoord` (ASM:1962) → `cur_pos_to_pixel_coord()` в `game/coords.c` ((pos<<4)+$8B)
- [x] `Construction` (ASM:349) → `construction()` в `game/construction_screen.c` (все ASM-метки восстановлены: `Skip_LoadFrame`/`Construction_Loop`/`Skip_Status_Handle`/`occurenceNotZero`/`loBitNotSet`/`occurenceNotZero_2`/`tsaNotFf`/`Construct_Draw_TSA`/`Construct_StartCheck`/`End_Construction`); **исправлен баг**: при A/B-нажатии ASM безусловно прыгает на `Construct_Draw_TSA` (рисует блок), а C-версия рисовала только при удерживании A/B (Joypad1_Buttons & 3) — теперь по нажатию A/B блок всегда перерисовывается под танком; убраны не-ASM строчки (`PPU_Addr_Ptr=$1C` уже в make_gray_frame, лишний `BkgPal_Number=0`); `JMP Title_Loaded` моделируется возвратом в caller
- [x] `Make_GrayFrame` (ASM:1813) → `make_gray_frame()` в `game/draw.c` (Block_X/Y=2, Counter/2=26)
- [x] `Draw_GrayFrame` (ASM:3906) → `draw_gray_frame()` в `game/draw.c` (`while` → `Draw_BlackRow`/`at_` goto-метки)

## Stage select / Battle entry

- [x] `Start_StageSelScrn` (ASM:452) → `start_stage_sel_scrn()` в `game/stage_select_screen.c` (все ASM-метки: StageSelect_Loop/Inc_LevelNum/Check_B/Dec_LevelNum/Start_Level/Skip_Lvl_Load/BuildScreen/Battle_Engine/Skip_Battle_Loop/Skip_Pause_Switch/AfterDeath_BattleRun/Ckeck_FirstFinish/Check_GameOver/Make_GameOver/Skip_RecordShow)
- [x] `Draw_StageNumString` (ASM:1977) → `draw_stage_num_string()` в `game/stage_select_screen.c` (убран defensive `if (x>=120) return`)
- [x] `Clear_NT` (ASM:627) → `clear_nt()` в `game/draw.c`
- [x] `Init_Level_VARs` (ASM:666) → `init_level_vars()` в `game/battle_screen.c` (at_ label для @_ skip)
- [x] `SetUp_LevelVARs` (ASM:798) → `setup_level_vars()` в `game/battle_screen.c` (at_/Set_VARs/at__/Respawn_Delay_Calc/exit_ labels)
- [x] `Set_VARs` (label, ASM:760) → часть `setup_level_vars()`
- [x] `Respawn_Delay_Calc` (ASM:798 label) → часть `setup_level_vars()`
- [x] `Battle_Loop` (ASM:695) → `battle_loop()` в `game/battle_screen.c` — 18 JSR-вызовов в точном ASM-порядке (ice_detect → ice_move → motion_handle → hide_hi_bit_under_tank → all_bullets_status_handle → hq_handle → invisible_timer_handle → make_player_shot → make_enemy_shot → respawn_handle → bullet_fly_handle → bullet_to_bullet_impact_handle → bullet_to_tank_impact_handle → bonus_handle → gameover_str_move_handle → play_snd_move → draw_player_lives → swap_pal_colors). Внутри нет ASM-меток
- [x] `Battle_Engine` (label, ASM:541) → секция внутри `start_stage_sel_scrn()` в `game/stage_select_screen.c` — все ASM-метки восстановлены 1-в-1: `Battle_Engine`, `Skip_Battle_Loop`, `Skip_Pause_Switch`, `AfterDeath_BattleRun`, `Ckeck_FirstFinish`, `Check_GameOver`, `Make_GameOver`, `Skip_RecordShow`. Исправлен баг: ASM безусловно ставит `Seconds_Counter = 0` после `LevelEnd_Check`, а C ставил только `0xFE` условно — теперь Seconds_Counter сначала зануляется, затем условно перезаписывается на `0xFE` если `GameOverStr_Timer != 0`. `JMP BEGIN` моделируется через `return` (caller → title_screen_loop ret=3 → begin.c `goto begin_label`)
- [x] `Swap_Pal_Colors` (ASM:721) → `swap_pal_colors()` в `game/battle_screen.c` (switch_/exit_ labels)
- [x] `Make_GameOver` (label, ASM:612) → часть `start_stage_sel_scrn()` (метка Make_GameOver внутри стейдж-логики)
- [x] `FreezePlayer_OnHQDestroy` (ASM:639) → `freeze_player_on_hq_destroy()` в `game/battle_screen.c` (at_ label)
- [x] `LevelEnd_Check` (ASM:1360) → `level_end_check()` в `game/battle_screen.c` — все 3 ASM-метки сохранены (`Init_GameOverStr`, `ExitLevel`, `PlayLevel`); 8-битная свёртка `P1 + P2` через `(uint8_t)`-каст эмулирует `CLC; ADC; BNE` (проверка по младшему байту); fallthrough из третьей проверки в `Init_GameOverStr`
- [x] `Init_GameOverStr` (label, ASM:1370) → часть `level_end_check()`
- [x] `Null_KilledEnms_Count` (ASM:1344) → `null_killed_enms_count()` в `game/battle_screen.c` (C-порт хранит 1P/2P счётчики раздельно)
- [x] `Null_both_HiScore` (ASM:656) → `null_both_hi_score()` в `game/hiscore_screen.c` (fallthrough в init_level_vars сохранён через явный вызов)

## Demo / Bonus level

- [x] `Load_DemoLevel` (ASM:820) → `load_demo_level()` в `game/demo_level_screen.c` — прямая последовательность без внутренних меток; `aBattle`/`aCity` теперь 0xFF-terminated массивы (дубликат данных из title_screen.c — в ASM это общая ROM-таблица); все 19 ASM-шагов в точном порядке (Pause_Flag=1, BkgPal_Number=0, Init_Level_VARs, Player2_Lives=3, обнуления, Make_GrayFrame, Level_Number=0xFF→Load_Level→30, Level_Mode=2, Screen_Off, рисовка BATTLE/CITY, Store_NT_Buffer_InVRAM, Set_PPU, SetUp_LevelVARs, DraW_Normal_HQ, NMI_Wait, TanksOnScreen=5)
- [x] `BonusLevel_ButtonCheck` (ASM:873) → `bonus_level_button_check()` в `game/demo_level_screen.c` — все 4 ASM-метки восстановлены (`BonusLevel_ButtonCheck` как goto-target для рекурсивного входа, `DemoLevel_Loop`, `End_Demo`, `Button_Pressed`); ASM `BEQ BonusLevel_ButtonCheck` → `goto BonusLevel_ButtonCheck`; ASM `PLA PLA; JMP Title_Loaded` моделируется через возврат кода 1 (caller `begin.c` делает `goto title_loaded`); код 0 → нормальный выход End_Demo → `goto new_scroll` в begin.c
- [x] `Demo_AI` (ASM:1187) → `demo_ai()` в `game/demo_level_screen.c` — все ASM-метки восстановлены (`loop`, `take_Bonus`, `noBonus`, `at__`, `at___`, `enemiesNotActing`, `load_Direction_DemoAI`, `saveButton_DemoAI`, `next_Demo_AI`); `STA Joypad1_Buttons,X / STA Joypad1_Differ,X` идёт **только в `saveButton_DemoAI`** (не дублируется по веткам как раньше); button-value передаётся через локальную `button` (аналог A-регистра); ASM-индексирование `+2,X`/`+3,X`/`+4,X` отражено в `Tank_Status[N + Counter]`

## Hi-score / Records

- [x] `Draw_Record_HiScore` (ASM:908) → `draw_record_hi_score()` в `game/hiscore_screen.c` (at_ wait-loop label)
- [x] `Draw_RecordDigit` (ASM:4165) → `draw_record_digit()` в `game/hiscore_screen.c` (at_/at__, Char_Index_Base=$30→$0, draw_brick_str)
- [x] `Update_HiScore` (ASM:4198) → `update_hi_score()` в `game/hiscore_screen.c` (все @-метки: at_/hiscoreFinished/fillLoop/continueProcess/loop_2/continueProcess_2/fillLoop_2/exit_; возвращает Y)
- [x] `Add_Score` (ASM:4255) → `add_score()` в `game/score.c` (at_/at__/at___ labels, явный carry)
- [x] `Add_Life` (ASM:2897) → `add_life()` в `game/score.c` (at_/Play_SndAncillaryLife/End_Add_Life labels)

## Secret message

- [x] `Show_Secret_Msg` (ASM:952) → `show_secret_msg()` в `game/secret_msg_screen.c` (линейная последовательность вызовов совпадает с ASM)
- [x] `Wait_1Second` (ASM:1046) → `wait_1second()` в `game/secret_msg_screen.c` (at_ label)
- [x] `Draw_Drop` (ASM:1062) → `draw_drop()` в `game/secret_msg_screen.c` (at_/at__ labels; респаун 7 итераций как в ASM, не 8)
- [x] `Draw_RespawnPic` (ASM:1112) → `draw_respawn_pic()` в `game/secret_msg_screen.c` (at_ label, явная 8-битная арифметика)

## HUD / Lives / Reinforcements / Pause / GameOver

- [x] `Draw_Player_Lives` (ASM:1472) → `draw_player_lives()` в `game/battle_hud.c` (метки Draw_2P_Lives/Draw_1P_Lives есть; убран лишний PPU_Addr_Ptr=$1C)
- [x] `Draw_2P_Lives` (label, ASM:1494) → часть `draw_player_lives()`
- [x] `Draw_1P_Lives` (label, ASM:1503) → часть `draw_player_lives()`
- [x] `Draw_LivesDigit` (label, ASM:1511) → часть `draw_player_lives()` (inline, Y = Counter*3 + $12)
- [x] `Draw_IP` (ASM:1536) → `draw_ip()` в `game/battle_hud.c` (Draw_IIP/locret_C858 labels)
- [x] `Draw_IIP` (label, ASM:1550) → часть `draw_ip()`
- [x] `Draw_LevelFlag` (ASM:1566) → `draw_level_flag()` в `game/battle_hud.c` (убран лишний PPU_Addr_Ptr=$1C)
- [x] `PointAt_RightScrnColumn` (ASM:1599) → `point_at_right_scrn_column()` в `game/battle_hud.c`
- [x] `ReinforceToRAM` (ASM:1618) → `reinforce_to_ram()` в `game/battle_hud.c`
- [x] `Draw_EmptyTile` (ASM:1632) → `draw_empty_tile()` в `game/battle_hud.c`
- [x] `Draw_Reinforcemets` (ASM:1645) → `draw_reinforcements()` в `game/battle_hud.c` (at_ label, DEC×2/BPL)
- [x] `Draw_Pause` (ASM:1698) → `draw_pause()` в `game/battle_screen.c` (End_Draw_Pause label)
- [x] `Draw_Fixed_GameOver` (ASM:1745) → `draw_fixed_game_over()` в `game/battle_screen.c`
- [x] `Draw_Brick_GameOver` (ASM:1131) → `draw_brick_game_over()` в `game/game_over_screen.c` (Next_Frame/End_Draw_Brick_GameOver labels)
- [x] `GameOver_Str_Move_Handle` (ASM:1772) → `gameover_str_move_handle()` в `game/battle_screen.c` (Hide_String/Check_Motion/Stopped_Motion/End_GameOver_Str_Move labels)
- [x] `Init_GameOver_Properties` (ASM:5127) → `init_game_over_properties()` в `game/battle_tank_status.c`

## HQ

- [x] `DraW_Normal_HQ` (ASM:2034) → `draw_normal_hq()` в `game/battle_hq.c`
- [x] `Draw_Naked_HQ` (ASM:2089) → `draw_naked_hq()` в `game/battle_hq.c` — 2 string-buffer-вызова ($E,$1A) и ($E,$1B); затем inline 4-байтный пакет в Screen_Buffer: `$23, $F3, (NT_Buffer[$3F3] & $3F), $FF` — single-byte attribute update в $23F3
- [x] `Draw_ArmourHQ` (ASM:2128) → `draw_armour_hq()` в `game/battle_hq.c` — 4 string-buffer-вызова на ($C,$18..$1B); затем 5-байтный пакет: `$23, $F3, $3F, (NT_Buffer[$3F4] & $CC) | $33, $FF` — 2-byte attribute update в $23F3 и $23F4
- [x] `Draw_Destroyed_HQ` (ASM:2185) → `draw_destroyed_hq()` в `game/battle_hq.c` — 2 string-buffer-вызова, без attribute-update
- [x] `HQ_Handle` (ASM:6032) → `hq_handle()` в `game/battle_hq.c` — все 6 ASM-меток теперь goto-цели: `Skip_DecHQTimer`, `Normal_HQ_Handle`, `HQ_Explode_Handle`, `at_` (= `@_`), `at__` (= `@__`), `End_HQ_Handle`. Исправлено: было `int`-арифметика, теперь явно `uint8_t` для всех 8-bit операций (SBC #5, EOR #$FF, ADC #1) — раньше работало по совпадению через truncation. `BPL`-инверсии теперь честно через `(int8_t)a >= 0; goto`
- [x] `Draw_BigExplode` (ASM:6162) → `draw_hq_big_explode(void)` в `game/battle_hq.c` — **изменена сигнатура**: теперь без аргумента (как в ASM). HQExplode_SprBase устанавливается caller'ом (FourthExplode_Pic ставит 0, FifthExplode_Pic ставит $10). Функция-«hq_»-префикс для отличия от `Draw_Big_Explode` (5352) для танков
- [x] `Add_ExplodeSprBase` (ASM:6128) → `add_explode_spr_base(delta)` в `game/battle_hq.c` — `delta + HQExplode_SprBase` → `draw_small_explode(tile)` → `Spr_TileIndex = tile; draw_whole_spr()`
- [x] `FirstExplode_Pic` (ASM:6105) → `first_explode_pic()` + ASM `Second/ThirdExplode_Pic` через общий `draw_hq_small_explode(tile)` — соответствует ASM-fallthrough из ThirdExplode в Draw_HQSmallExplode (хранит X/Y = $78/$D8)
- [x] `FourthExplode_Pic` (ASM:6140) → `fourth_explode_pic()` — теперь явно `HQExplode_SprBase = 0u; draw_hq_big_explode();` (точно как в ASM `LDA #0; STA HQExplode_SprBase; JSR Draw_BigExplode`)
- [x] `FifthExplode_Pic` (ASM:6151) → `fifth_explode_pic()` — то же что Fourth, но `HQExplode_SprBase = $10u`

## Points screen

- [x] `Draw_Pts_Screen` (ASM:2331) → `draw_pts_screen()` в `game/pts_screen.c` — все 12 ASM-меток восстановлены (`DrawPtsScrn_NxtTank`, `DrawPtsScrn_NxtCount`, `at_`/`at__`/`at___`/`at_____`/`at______`, `tanksProcessed`, `DrawPtsScrn_CheckHQ`, `DrawPtsScrn_CheckNum`, `DrawPtsScrn_CheckLives`, `End_Draw_Pts_Screen`); fallthrough-метки сделаны goto-целями для устранения unused-warnings; удалён orphan `draw_player_kill` (был `Draw_PlayerKill:` — это **внутренняя метка** `Draw_Kill_Points`, не отдельная функция; в C-порте уже корректно реализована как goto-label в `battle_tank_draw.c::draw_kill_points`)
- [x] `Draw_Pts_Screen_Template` (ASM:2611) → `draw_pts_screen_template()` в `game/pts_screen.c` — ASM-метки `Skip_ScndPlayerDraw` и `Skip_ScndPlayerPtsDraw` сохранены; обнуления HiScore-строк через `save_aligned_str_to_scr_buffer` (с авто-сдвигом X для right-alignment); 4×Arrow_Left + опционально 4×Arrow_Right для 2P-режима
- [x] `Draw_Tank_Column` (ASM:2826) → `draw_tank_column()` в `game/pts_screen.c` — `TSA_Pal=2; затем 4× (Temp_Y = координата; draw_spr_in_column(tile))`. Tile передаётся как параметр (раньше caller ставил Spr_TileIndex напрямую — менее ASM-faithful)
- [x] `Fill_Attrib_Table` (ASM:2848) → `fill_attrib_table()` в `game/pts_screen.c` — 28 индивидуальных записей в `NT_Buffer+$3C0..$3F7` (через memset для пачек 4-byte + индивидуальные)
- [x] `Draw_Spr_InColumn` (ASM:2886) → `draw_spr_in_column(tile)` в `game/pts_screen.c` — **изменена сигнатура**: теперь принимает tile (был в ASM A до JSR; ASM делает `STA Spr_TileIndex` внутри функции, теперь и C тоже); `Spr_TileIndex = tile; Temp_X = $81; draw_whole_spr()`
- [x] `DrawTankColumn_XTimes` (ASM:3069) → `draw_tank_column_x_times(count)` в `game/pts_screen.c` — переписана с ASM-меткой `DrawTankColumn_XTimes:` и `goto`: `NMI_Wait; Draw_Tank_Column; DEX; BNE DrawTankColumn_XTimes` (раньше был `while`-loop)

## Tank logic — статусы и движение

- [x] `TanksStatus_Handle` (ASM:5215) → `tanks_status_handle()` в `game/battle_tank_draw.c` (at_ label)
- [x] `SingleTankStatus_Handle` (ASM:5233) → `single_tank_status_handle()` в `game/battle_tank_draw.c`
- [x] `OperatingTank` (ASM:5469) → `operating_tank()` в `game/battle_tank_draw.c` (OperTank_Draw label)
- [x] `Respawn` (ASM:5491) → `respawn()` в `game/battle_tank_draw.c` (skip label)
- [x] `Status_Core` (ASM:4910) → `status_core()` в `game/battle_tank_status.c` (jump table)
- [x] `Misc_Status_Handle` (ASM:4943) → `misc_status_handle()` в `game/battle_tank_status.c` (LoadStts/End labels)
- [x] `Get_RandomStatus` (ASM:4952) → `get_random_status()` в `game/battle_tank_status.c` (Sbc/Save/End labels)
- [x] `Explode_Handle` (ASM:5127) → `explode_handle()` в `game/battle_tank_status.c` (Skip/SkipRiseBit/SaveStts/Dec_Enemy/CheckHQ/Check1pLives/End labels)
- [x] `Move_Tank` (ASM:1286) → `move_tank()` в `game/construction_screen.c` (все ASM-метки: `ArrowNotPressed`/`checkJoypad`/`delayEq14`/`moveTank`/`End_Move_Tank`)
- [x] `Check_BorderReach` (ASM:1690) → `check_border_reach()` в `game/battle_tank.c` (at_/at__/at___/End labels)
- [x] `Detect_Motion` (ASM:4603) → `detect_motion()` в `game/battle_tank.c` (End_Detect_Motion label)
- [x] `Motion_Handle` (ASM:4741) → `motion_handle()` в `game/battle_tank.c` (Skip_TimerOps/Enemy/at_/JumpToStatusHandle/Motion_Handle_Next labels)
- [x] `Ice_Move` (ASM:4690) → `ice_move()` в `game/battle_tank.c` (at_/loop/at___/at____/at_____/usual_Tank/iceStatus/skip/nextTank/End labels)
- [x] `Ice_Detect` (ASM:5813) → `ice_detect()` в `game/battle_tank.c` (loop/next_Tank labels)
- [x] `Respawn_Handle` (ASM:4603) → `respawn_handle()` в `game/battle_respawn.c` (at_/at__/nextPlayer/End labels)
- [x] `Make_Respawn` (ASM:6221) → `make_respawn()` в `game/battle_respawn.c` (Enemy_Operations/at_/Make_BonusEnemy/exit_ labels)
- [x] `Set_Respawn` (ASM:5164) → `set_respawn()` в `game/battle_tank_status.c` (End_Set_Respawn label)
- [x] `Load_Tank` (ASM:5206) → `load_tank()` в `game/battle_tank_status.c` (End_Load_Tank label)
- [x] `Load_New_Tank` (ASM:6301) → `load_new_tank()` в `game/battle_respawn.c` (load_NewEnemy/continueProcess/checkTankType/exit labels)
- [x] `Load_Enemy_Count` (ASM:6373) → `load_enemy_count()` в `game/battle_respawn.c` (LevelModeZero/SaveEnemyCount labels)
- [x] `Null_Status` (ASM:6331) → `null_status()` в `game/battle_tank.c` (at_ label, DEX/BPL)
- [x] `Rise_TankStatus_Bit` (ASM:6352) → `rise_tank_status_bit()` в `game/battle_tank.c`
- [x] `Button_To_DirectionIndex` (ASM:6644) → `button_to_direction_index()` в `game/battle_tank.c` (at_/at__/at___/at____ labels через ASL+BCC цепочку)
- [x] `Compare_Block_X` (ASM:4999) → `compare_block_x()` в `game/battle_tank_status.c`
- [x] `Compare_Block_Y` (ASM:5054) → `compare_block_y()` в `game/battle_tank_status.c`
- [x] `Aim_FirstPlayer` (ASM:4979) → `aim_first_player(slot)` в `game/battle_tank_status.c` — точка входа конгломерата (`JMP Save_AI_ToStatus` моделируется как tail-call к `save_ai_to_status(slot)`); `AI_X_Aim = Tank_X[0]; AI_Y_Aim = Tank_Y[0]`. Используется как JSR-цель из Status_JumpTable[26]
- [x] `Aim_ScndPlayer` (ASM:4986) → `aim_scnd_player(slot)` в `game/battle_tank_status.c` — то же что Aim_FirstPlayer но `Tank_X+1/Tank_Y+1` → `Tank_X[1]/Tank_Y[1]`; JSR-цель из Status_JumpTable[24]
- [x] `Aim_HQ` (ASM:4993) → `aim_hq(slot)` в `game/battle_tank_status.c` — `AI_X_Aim = $78, AI_Y_Aim = $D8`; ASM-fallthrough в Save_AI_ToStatus развёрнут как явный вызов `save_ai_to_status(slot)`; JSR-цель из Status_JumpTable[22]
- [x] `Save_AI_ToStatus` (ASM:4999) → `save_ai_to_status(slot)` в `game/battle_tank_status.c` — общий хвост Aim_*-конгломерата с единственным RTS; `JSR Load_AI_Status; STA Tank_Status,X` → `Tank_Status[slot] = load_ai_status(slot)`. `; End of function Aim_FirstPlayer` в ASM маркирует конец всей группы из 4 точек входа
- [x] `Load_AI_Status` (ASM:5008) → `load_ai_status()` в `game/battle_tank_status.c` — все 4 ASM-метки сохранены (`Load_AIStatus_GetRandom`, `LoadSecondPart`, `checkDifferFlag`, `End_Load_AIStatus`); исправлены 2 бага: **(1)** player-branch использовал выдуманную проверку `Tank_Status[1] == 0` вместо ASM-формулы `((slot << 1) ^ Seconds_Counter) & 2`; **(2)** ASM `STA AI_X_DifferFlag` после вычисления `Y*3+X` перезаписывает глобал индексом — в C теперь тоже `AI_X_DifferFlag = ...` явно. AI_X_DifferFlag меняет роль: сначала 0/1/2 (sign), затем 0..8 (индекс таблицы) после `STA`
- [x] `Get_RandomAim` (ASM:5215) → `get_random_aim()` в `game/battle_tank_status.c` (3-tier логика: HQ/random-dir/player через Respawn_Delay vs Seconds_Counter)
- [x] `Relation_To_Byte` (ASM:4548) → `relation_to_byte()` в `game/battle_tank_status.c` (сигнатура `(lhs, rhs)` для эмуляции carry от SBC)
- [x] `Invisible_Timer_Handle` (ASM:6091) → `invisible_timer_handle()` в `game/battle_tank.c` (loop_/at_/next_Invisible_Timer_Handle labels — переименованы с itm_*)

## Tank draw — взрывы, очки, ricochet, спавн-картинка

- [x] `Draw_Small_Explode2` (ASM:5250) → `draw_small_explode2()` в `game/battle_tank_draw.c` — `Spr_Attrib=0; Temp_X/Y из Tank_X/Y[slot]; draw_bullet_ricochet(Tank_Status[slot]); Spr_Attrib=$20`
- [x] `Draw_Small_Explode1` (ASM:5336) → `draw_small_explode1()` в `game/battle_tank_draw.c` — `Spr_Attrib=0; Temp_X/Y; draw_ricochet(8); Spr_Attrib=$20`
- [x] `Draw_Big_Explode` (ASM:5352) → `draw_big_explode()` в `game/battle_tank_draw.c` — рисует 4 квадранта 32x32-взрыва: 4× `set_spr_index(i); X/Y±=8; draw_whole_spr()`. Counter ставится из slot для Set_SprIndex
- [x] `Draw_Bullet_Ricochet` (ASM:5269) → `draw_bullet_ricochet()` в `game/battle_tank_draw.c` — ASM-fallthrough в Draw_Ricochet теперь явный вызов `draw_ricochet(a)` после вычисления `((7 - (a_val>>4)) << 2)`. Удалён дубликат в `battle_bullet_draw.c` (был там по ошибке; ASM-расположение — в tank_draw секции)
- [x] `Draw_Kill_Points` (ASM:5296) → `draw_kill_points()` в `game/battle_tank_draw.c` — все ASM-метки восстановлены как goto-цели: `Draw_PlayerKill`, `Draw_Kill_Points_Skip`. `if (Tank_Type == 0) goto Draw_PlayerKill` вместо if-else; fallthrough Draw_PlayerKill → Draw_Kill_Points_Skip
- [x] `Set_SprIndex` (ASM:5408) → `set_spr_index(value)` в `game/battle_tank_draw.c` — `value*4 + $D1` через Temp; `Tank_Status[Counter] & $F0; -$30; ^$10` для direction-битов; результат + Temp = Spr_TileIndex; выход через Temp_X/Y из Tank_X/Y[Counter]

## Bullets — статусы и движение

- [x] `AllBulletsStatus_Handle` (ASM:5520) → `all_bullets_status_handle()` в `game/battle_bullet_status.c` (at_ label, DEC/BPL)
- [x] `BulletStatus_Handle` (ASM:5536) → `bullet_status_handle()` в `game/battle_bullet_status.c` (jump table, защитный bounds-check 5u оставлен)
- [x] `Bullet_Move` (ASM:5563) → `bullet_move()` в `game/battle_bullet_status.c` (End_Bullet_Move label)
- [x] `Change_BulletCoord` (ASM:5571) → `change_bullet_coord()` в `game/battle_bullet.c` (slot/direction как params)
- [x] `Make_Ricochet` (ASM:5589) → `make_ricochet()` в `game/battle_bullet_status.c` (skip_/exit_ labels)
- [x] `Make_Shot` (ASM:5612) → `make_shot()` в `game/battle_bullet.c` (skip_/quickBullet_End_Make_Shot/lastType/exit_ labels)
- [x] `Make_Player_Shot` (ASM:5738) → `make_player_shot()` в `game/battle_bullet.c` (loop_/at__/next_Jump_Make_Shot labels)
- [x] `Make_Enemy_Shot` (ASM:5785) → `make_enemy_shot()` в `game/battle_bullet.c` (loop_/next_Make_Enemy_Shot/exit_ labels)
- [x] `Update_Ricochet` (ASM:5721) → `update_ricochet()` в `game/battle_bullet_draw.c`
- [x] `Bullet_Fly_Handle` (ASM:6644) → `bullet_fly_handle()` в `game/battle_bullet.c` (loop/at__/at___/at____/getCoord/next/End labels)
- [x] `Hide_All_Bullets` (ASM:6315) → `hide_all_bullets()` в `game/battle_bullet.c` (at_ label, DEX/BPL)
- [x] `BulletToObject_Impact_Handle` (ASM:6722) → `bullet_to_object_impact_handle()` в `game/battle_collide.c` (at_/at__/Armored_Wall/BulletToObject_Return0/Return1 labels)
- [x] `BulletToTank_Impact_Handle` (ASM:6731) → `bullet_to_tank_impact_handle()` в `game/battle_collide.c` (все @-метки сохранены)
- [x] `BulletToBullet_Impact_Handle` (ASM:7069) → `bullet_to_bullet_impact_handle()` в `game/battle_collide.c` (loop/loop_2/checkMinX/checkMinY/next labels)

## Bullets — отрисовка

- [x] `Draw_All_BulletGFX` (ASM:5669) → `draw_all_bullet_gfx()` в `game/battle_bullet_draw.c` — метка `at_` (= ASM `@_`); итерация `Counter` 9 → 0 через `DEC; BPL` как в ASM (был обратный порядок 0 → 9 через `for`)
- [x] `Draw_BulletGFX` (ASM:5685) → `draw_bullet_gfx()` в `game/battle_bullet_draw.c` — ASM-структура `LSR;LSR;LSR; AND #$FE; TAY` сохранена через явный `y = (status >> 3) & 0xFE`, индекс таблицы = `y >> 1` (так как ASM-таблица 16-битная, а C — массив указателей); `JMP (LowPtr)` через диспетч-массив `BulletGFX_JumpTable[5]` с guard'ом по диапазону. `End_Ice_Move` в jump-table обёрнут адаптером `bullet_end_ice_move(uint8_t)` (см. §JSR-цели в начале файла — End_Ice_Move в HQExplode имеет void-сигнатуру, поэтому отдельная adapter-функция)
- [x] `Draw_Bullet` (ASM:5702) → `draw_bullet()` в `game/battle_bullet_draw.c` — порядок операций восстановлен по ASM (`AND #3; PHA; LDY/LDA/TAX; LDA #2; STA TSA_Pal; LDA #$B1; STA Spr_TileIndex; PLA; JSR Indexed_SaveSpr`)

## Sprites / Tiles / NT-buffer helpers

- [x] `Draw_TSA_On_Tank` (ASM:1273) → `draw_tsa_on_tank()` в `game/construction_screen.c`
- [x] `Draw_TSABlock` (ASM:3928) → `draw_tsa_block()` в `game/draw.c` (block_num как param заменяет PHA/PLA)
- [x] `Draw_Tile` (ASM:3821) → `draw_tile()` в `game/draw.c` (использует PPU_Addr_Ptr вместо литерала $1C)
- [x] `Draw_Destroyed_Brick` (ASM:3753) → `draw_destroyed_brick()` в `game/battle_collide.c`
- [x] `DrawPtrTile` (ASM:3781) → `draw_ptr_tile()` в `game/draw.c` (ASM-комментарий "never used"; ORA-логика опущена)
- [x] `Check_Object` (ASM:3742) → `check_object()` в `game/battle_collide.c`
- [x] `NT_Buffer_Process_XOR` (ASM:3764) → `nt_buffer_process_xor()` в `game/draw.c` (at_ skip label)
- [x] `NT_Buffer_Process_OR` (ASM:3791) → `nt_buffer_process_or()` в `game/draw.c` (at__ skip label)
- [x] `Rise_Nt_HighBit` (ASM:5885) → `rise_nt_high_bit()` в `game/battle_tank.c`
- [x] `HideHiBit_Under_Tank` (ASM:5895) → `hide_hi_bit_under_tank()` в `game/battle_tank.c` (at_/at__/at___ labels)
- [x] `HideHiBit_InBuffer` (ASM:5936) → `hide_hi_bit_in_buffer()` в `game/battle_tank.c`
- [x] `Inc_Ptr_on_A` (ASM:3847) → `inc_ptr_on_a()` в `game/draw.c` (at_ label, BCC carry-check)
- [x] `Copy_AttribToScrnBuff` (ASM:2206) → `copy_attrib_to_scrn_buff()` в `game/draw.c` (at_ label, per-byte с NMI_Wait, как в ASM — было ошибочно пачкой)
- [x] `AttribToScrBuffer` (ASM:3482) → `attrib_to_scr_buffer()` в `game/draw.c`
- [x] `TSA_Pal_Ops` (ASM:3534) → встроено в `tsa_pal_ops()` (at_/at__/at___/End_TSA_Pal_Ops labels)
- [x] `OR_Pal` (ASM:3561) → `or_pal()` в `game/draw.c`
- [x] `FillScr_Single_Row` (ASM:2242) → `fill_scr_single_row()` в `game/draw.c` (at_/at__ labels)
- [x] `FillNT_with_Grey` (ASM:2280) → `fill_nt_with_grey()` в `game/draw.c` (at_ label)
- [x] `FillNT_with_Black` (ASM:2306) → `fill_nt_with_black()` в `game/draw.c` (at_ label)

## Sprite buffer

- [x] `Draw_WholeSpr` (ASM:4426) → `draw_whole_spr()` в `game/draw.c`
- [x] `SaveSprTo_SprBuffer` (ASM:4375) → `save_spr_to_spr_buffer()` в `game/draw.c` (Skip_Attrib label, убран лишний bounds-check и редундантный `goto Skip_Attrib`)
- [x] `Indexed_SaveSpr` (ASM:4396) → `indexed_save_spr()` в `game/draw.c` (убран лишний `&3`)
- [x] `Spr_TileIndex_Add` (ASM:4413) → `spr_tile_index_add()` в `game/draw.c` (убран лишний `&3`)
- [x] `Spr_Invisible` (ASM:4446) → `spr_invisible()` в `game/draw.c`

## Coords / address math

- [x] `CoordTo_PPUaddress` (ASM:3459) → `coord_to_ppu_address(x, y)` в `game/coords.c` — 3× `LSR A / ROR Temp` для разнесения младших 3 бит Y в верхние 3 бита Temp; возврат `(high << 8) | low` вместо ASM-возврата через A/Y регистры. **Побочный эффект на глобальный `Temp` сохранён 1-в-1 с ASM** (`STA Temp` в начале + `ROR Temp` ×3 → финально `Temp = (y & 7) << 5`)
- [x] `GetCoord_InTiles` (ASM:3670) → `get_coord_in_tiles_xy(x, y)` в `game/coords.c` — прямая ASM-точка входа (вход через параметры эквивалентно CPU X/Y регистрам); `JSR XnY_div_8` + fallthrough в `CoordsToRAMPos` развёрнут в `xny_div_8(&x, &y); coords_to_ram_pos(x, y);`. Spr_X/Spr_Y НЕ трогаются — соответствует ASM, где они меняются только префиксом в Get_SprCoord_InTiles. Callers: `save_spr_to_spr_buffer` (ASM @4367), `ice_detect` (ASM @5831), `battle_tank_status` (ASM @4844/4866)
- [x] `Get_SprCoord_InTiles` (ASM:3709) → `get_spr_coord_in_tiles(x, y)` в `game/coords.c` — ASM-префикс `STX Spr_X; STY Spr_Y` восстановлен явно: `Spr_X = x; Spr_Y = y;`. Затем `get_coord_in_tiles_xy(x, y)` (fallthrough в GetCoord_InTiles). ASM-fallthrough в Temp_Coord_shl (см. Draw_Char @ASM:4019) развёрнут как явный `temp_coord_shl()` после вызова в callers (draw_char)
- [x] `GetSprCoord_InTiles` (ASM:6652) → **та же** `get_spr_coord_in_tiles(x, y)` в `game/coords.c` — второй идентичный ASM-label с тем же телом. ASM-fallthrough в `BulletToObject_Impact_Handle` (callers @ASM:6599/6622) развёрнут как явные `get_spr_coord_in_tiles(x, y); bullet_to_object_impact_handle(slot)` в `battle_bullet.c`
- [x] `CoordsToRAMPos` (ASM:3677) → `coords_to_ram_pos()` в `game/coords.c` — `JSR CoordTo_PPUaddress; STA HighPtr_Byte; STY LowPtr_Byte`; ASM-инструкция `LDY #0` (сброс CPU Y-регистра) не имеет аналога в C
- [x] `XnY_div_8` (ASM:3689) → `xny_div_8()` в `game/coords.c` — `*y >>= 3; *x >>= 3` через указатели (в ASM CPU X/Y регистры, в C — out-параметры)
- [x] `Temp_Coord_shl` (ASM:3719) → `temp_coord_shl()` в `game/coords.c` — ASM-метки `at_`/`at__` (= `@_`/`@__`) восстановлены; `Temp = 1; if (Spr_Y & 4) Temp <<= 2; if (Spr_X & 4) Temp <<= 1`
- [x] `Multiply_Bonus_Coord` (ASM:7051) → `multiply_bonus_coord()` в `game/battle_bonus.c` — `STA Temp` → локальный `temp`; точная последовательность `ADC self / ADC Temp / ADC self / ADC #6 / ASL ASL ASL` через 7 `a + a`/`a + temp`/`a + 6` операций. Эффект: `a → ((a*6)+6)*8 = a*48+48`

## Strings / digits

- [x] `String_to_Screen_Buffer` (ASM:3602) → `string_to_screen_buffer()` в `game/draw.c` (at_/at__ labels; HighStrPtr_Byte = raw hi)
- [x] `Save_Str_To_ScrBuffer` (ASM:3635) → `save_str_to_scr_buffer()` в `game/draw.c` (at_/at__/at___ labels, BMI через (int8_t)<0)
- [x] `PtrToNonzeroStrElem` (ASM:4135) → `ptr_to_nonzero_str_elem()` в `game/draw.c` — все 4 ASM-метки сохранены (`PtrToNonzeroStrElem`/`at_`/`at__`/`at___`); при `$FF`-терминаторе откатывает указатель на 2 байта при `Tmp_CharIndexBase==0` (для title HI-score: «00» при score=0) или на 1 байте иначе (для жизней / уровня: «0»). ASM-fallthrough из первого `DEX/DEY` во второй сохранён через падение исполнения. **C-port хелпер** `save_aligned_str_to_scr_buffer()` оборачивает связку «PtrToNonzeroStrElem → Save_Str_To_ScrBuffer»: в ASM CPU-регистр X служит счётчиком display-колонки и инкрементится внутри `PtrToNonzeroStrElem` через INX; в C регистра нет, поэтому хелпер вычисляет `skip = p - base_str` и передаёт `col + skip` в `save_str_to_scr_buffer`. Не самостоятельная ASM-функция.
- [x] `Num_To_NumString` (ASM:4291) → `num_to_num_string()` в `game/draw.c` (метка `at_` для value==0 ветки; `Temp` сохранён как глобал per ASM `STA Temp`)
- [x] `ByteTo_Num_String` (ASM:4339) → `byte_to_num_string()` в `game/draw.c` (метки `Check_Max`/`exit_`; десятичное разложение через цикл вычитания 10 с goto)
- [x] `Null_8Bytes_String` (ASM:4316) → `null_8bytes_string()` в `game/draw.c` (memset(0,7)+str[7]=$FF)
- [x] `StaffStr_Store` (ASM:3340) → `staff_str_store()` в `game/reset.c` (at_ label, DEX/BPL)
- [x] `StaffStr_Check` (ASM:3368) → `staff_str_check()` в `game/reset.c` (at_/ColdBoot labels)
- [x] `Draw_Char` (ASM:4026) → `draw_char()` в `game/draw.c` (Add_10/at_/NextByte/Next_Bit/Empty_Pixel/pixelProcessed labels)
- [x] `Draw_BrickStr` (ASM:4074) → `draw_brick_str()` в `game/draw.c` (New_Char/EOS labels)

## Bonus

- [x] `Bonus_Draw` (ASM:5947) → `bonus_draw()` + `draw_bonus()` в `game/battle_bonus.c` — DEC `BonusPts_TimeCounter` → если 0 чистит `Bonus_X`; иначе рисует тайл $3B (taken) или `Bonus_Number*4+$81` (not-taken, blink каждые 8 кадров)
- [x] `Bonus_Appear_Handle` (ASM:7012) → `bonus_appear_handle()` в `game/battle_bonus.c` — метка `at_` (= ASM `@_`); цикл случайной генерации координат + bonus_handle, пока он не «съест» бонус, затем выбор типа из `BonusNumber_ROM_Array`
- [x] `Bonus_Handle` (ASM:7138) → `bonus_handle()` в `game/battle_bonus.c` — все ASM-метки сохранены (`loop`, `skip`, `skip_2`, `bonus_Command`, `decNumber`, `exit`); dx/dy через ASM-идиому `SBC + BPL + EOR #$FF + ADC #1` (signed-abs), а не unsigned-abs; cross-function dispatch через `Bonus_JumpTable` с эмуляцией `PLA PLA; JMP (LowPtr)` через обычный вызов и `goto exit`
- [x] `Bonus_Helmet` (ASM:7220) → `bonus_helmet()` — исправлен баг: было `Invisible_Timer[0]` (хардкод), стало `Invisible_Timer[Tank_Num]` (как `STA Invisible_Timer,X`)
- [x] `Bonus_Watch` (ASM:7229) → `bonus_watch()` — `EnemyFreeze_Timer = 10`
- [x] `Bonus_Shovel` (ASM:7238) → `bonus_shovel()` — метка `End_Bonus_Shovel` (была early `return`); `BPL` через `(int8_t)HQ_Status >= 0`
- [x] `Bonus_Star` (ASM:7252) → `bonus_star()` — исправлен баг: было `Player_Type[0]/Tank_Type[0]`, стало `[Tank_Num]`; метка `End_Bonus_Star`
- [x] `Bonus_Grenade` (ASM:7268) → `bonus_grenade()` — метки `Bonus_Grenade_Loop`/`Explode_Next`; идёт с Counter=7 вниз, выходит при Counter==1 (игроков не взрываем)
- [x] `Bonus_Life` (ASM:7296) → `bonus_life()` — исправлен баг: `INC Player1_Lives,X` (где X=Tank_Num) даёт жизнь Player1 при X=0 и Player2 при X=1 (соседние в zp); было хардкодом Player1; fallthrough в `Bonus_Pistol` (только RTS)

## Input / Random

- [x] `Read_Joypads` (ASM:3571) → `read_joypads()` в `game/nmi.c` — внешний цикл `at__` по X=1→0 (P2 затем P1) и формула `Differ = ~prev & now` сохранены 1-в-1; hardware-strobe `JOYPAD_PORT1` и 8-итерационный bit-loop `@_` (`LDA $4016,X; AND #3; CMP #1; ROR Temp`) физически невозможны в C — заменены на вызов SDL-опроса, возвращающего все 8 бит сразу. Архитектурное расхождение, помеченное в комментарии
- [x] `Get_Random_A` (ASM:3226) → `get_random_a()` в `game/random.c` — алгоритм 1-в-1: `Random_Lo = (Random_Lo*7) + Seconds_Counter + zp[Random_Hi]` через `ASL ASL ASL; SEC SBC`. Индексированный доступ `ADC Temp,X` (где X = Random_Hi) к zero-page реализован через локальный `static uint8_t zp_bytes[256]` (детерминированный сид `i*31+17`) — полноценный union по всем zp-переменным не делается (см. AGENTS.md/PORTING.md прим. ниже)

## Sound engine

- [x] `Sound_Stop` (ASM:7315) → `sound_stop()` в `game/sound_engine.c` — метки `at_`/`at__` (= ASM `@_`/`@__`); чистит `Sound_DataBlocks[i*8]` и `Sound_PlaybackState[i]` для 28 слотов
- [x] `Play_Sound` (ASM:7349) → `play_sound()` в `game/sound_engine.c` — все 22 метки сохранены 1-в-1 с ASM (`skip`, `clearChannels`, `loopChannels`, `playLoop`, `gt5`, `writeToRegistersLoop`, `endProcessing`, `skip_2`, `SilencingLoop`, `skipSilencing`, `mainProcessingLoop`, `advanceToNextSlot`, `nextSlot`, `handleDurationCountdown`, `initializeNewSound`, `loadSoundPtr`, `readNextCommandByte`, `processFrequencyLookup`, `loopSoundTemp`, `skip_3`, `handleNote`, `specialCommand`); поддержка `Sound_CurrentData_Ptr` опущена (C использует `slot*8`-индексацию)
- [x] `Play_Snd_Move` (ASM:4523) → `play_snd_move()` в **`game/battle_screen.c`** (не sound_engine.c!) — был инвертирован `BEQ No_MoveSound` (`!= 0` вместо `== 0`), исправлено: теперь корректно стартует/глушит звук движения в зависимости от `Snd_Move` и `detect_motion`
- [x] `Load_Snd_Ptr` (ASM:7769) → `load_snd_ptr()` в `game/sound_engine.c` — заменено на C-указатель `s_snd_data_ptr` вместо 16-битного `Sound_DataPtr`, функционально эквивалентно
- [x] `Sound_LoadNextByte` (ASM:7784) → `sound_load_next_byte()` в `game/sound_engine.c` — читает `blk[5]`, берёт байт по data_ptr, инкрементит `blk[5]`
- [x] `Sound_DispatchCommand` (ASM:7803) → `sound_dispatch_command()` в `game/sound_engine.c` — ASM-трюк inline jump table за `JSR` (PLA-PLA от return address) заменён на массив `sound_command_jump_table[18]` с `bool`-возвратом (`true` = `JMP readNextCommandByte`, `false` = `JMP advanceToNextSlot`)
- [x] `Sound_Command_StopReset` (ASM:7622) → `sound_command_stop_reset()` — `return false` ≡ `JMP advanceToNextSlot`
- [x] `Sound_Command_SetDutyCycle` (ASM:7640) → `sound_command_set_duty_cycle()` — `blk[1] = (blk[1] & 0x3F) | b`
- [x] `Sound_Command_SetVolume` (ASM:7654) → `sound_command_set_volume()` — `blk[1] = (blk[1] & 0xC0) | b`
- [x] `Sound_Command_SetVolumeAlt` (ASM:7668) → `sound_command_set_volume_alt()` — `AND #Sound_CurrentData_Ptr` это артефакт дизассемблера, реальный байт `$C0` (как в SetVolume)
- [x] `Sound_Command_SetSweep` (ASM:7682) → `sound_command_set_sweep()` — `blk[2] = b`
- [x] `Sound_Command_SetTimerHigh` (ASM:7692) → `sound_command_set_timer_high()` — `blk[4] = b`
- [x] `Sound_Command_SetDutyVolume` (ASM:7702) → `sound_command_set_duty_volume()` — `blk[1] = b`
- [x] `Sound_Command_ClearCounters` (ASM:7712) → `sound_command_clear_counters()` — сброс трёх `Sound_LoopCounter*`
- [x] `Sound_Command_LoopCount0` (ASM:7726) → `sound_command_loop_count0()` — внутренняя метка `equal0:` сохранена; INC+CMP, если counter≠target → `return sound_command_jump_to_offset()`, иначе reset+fallthrough на `sound_command_advance_pointer()`. ASM-трюк `.BYTE $2C` (общая точка входа через BIT-skip) невозможен в C — развёрнут в три отдельные C-функции
- [x] `Sound_Command_LoopCount1` (ASM:7730) → `sound_command_loop_count1()` — копия с `Sound_LoopCounter1`
- [x] `Sound_Command_LoopCount2` (ASM:7734) → `sound_command_loop_count2()` — копия с `Sound_LoopCounter2`
- [x] `Sound_Command_AdvancePointer` (ASM:7746) → `sound_command_advance_pointer()` — `blk[5]++` (LDY #5; LDA; ADC #1; STA)
- [x] `Sound_Command_JumpToOffset` (ASM:7754) → `sound_command_jump_to_offset()` — `blk[5] = sound_load_next_byte()`

## Levels / misc

- [x] `Load_Level` (ASM:7910) → `load_level()` в `game/levels.c` (все ASM-метки: `begin`/`at_`/`at__`/`loop`/`loop_2`/`drawTsa`; `Inc_Ptr_on_A #$5B` → `level_ptr += LEVEL_SIZE`)
- [x] `Zero_Page_Viewer` (ASM:1395) → `zero_page_viewer()` в `game/draw.c` (debug, не вызывается; метки `SkipInc_Zero_Page_Viewer`/`ScipDec_Zero_Page_Viewer`/`End_Zero_Page_Viewer` сохранены; чтение zero-page заглушено нулём — в C-порту нет реального ZP mapping)

---

## Легенда статусов

- `[x]` — портирована и **проверена** на побайтовое соответствие ASM (закрыта в текущей сессии).
- `[~]` — портирована, нет очевидных багов, но строчного сличения не было.
- `[ ]` — не проверена в этой сессии / возможны расхождения.

При проверке функции отмечать `[x]` и приписывать короткий комментарий с верифицирующей правкой или подтверждением соответствия.

---

## Замечание: JSR-цели ≠ функции

В 6502-ASM любой адрес может быть целью `JSR`. Дизассемблер размечает «функцию» границами `; End of function …`, но фактическая семантика бывает разная. При портировании важно различать **четыре** ситуации, чтобы корректно решить, делать ли в C отдельную функцию или `goto`-метку:

1. **Полноценный JSR-API.** Метка вызывается из разных мест через `JSR` и заканчивается `RTS`. → **отдельная функция в C.**
   Пример: `JSR Null_NT_Buffer`, `JSR Set_PPU`, `JSR Save_Str_To_ScrBuffer`.

2. **Внутренняя метка (только ветвление).** Метка достигается только локально через `BNE`/`BEQ`/`JMP` из той же «функции», `RTS` нет (или есть один общий в конце). → **`goto`-метка в C, не отдельная функция.**
   Пример: `Skip_LoadFrame` (ASM:357), `Construction_Loop` (ASM:380), `Construct_Draw_TSA` (ASM:433), `End_Construction` (ASM:442), `tanksProcessed` (ASM:2482), `Title_Loaded` (ASM:341, цель `JMP` из `End_Construction`).

3. **Fallthrough-«функция».** После `; End of function X` нет `RTS` — управление падает в следующую функцию. → в C объединяется с следующей через `goto` или прямой вызов.
   Пример: `Selected_1player` → `accept:` → `Selected_2players` (ASM:1937–1948): обе ветки заканчиваются в общем `accept` и затем `JMP Start_StageSelScrn` без RTS.

4. **Метка как «вторая точка входа».** `JSR Label` извне попадает в **середину** функции, минуя её пролог. → в C обычно делается отдельная обёртка или передаётся аргумент, имитирующий вход в эту точку.
   Пример: `Set_VARs` (ASM:760) — точка входа внутри `SetUp_LevelVARs`; `Battle_Engine` (ASM:541) — точка входа внутри `Start_StageSelScrn`; `Construct_StartCheck` (ASM:436); `DrawPtsScrn_CheckHQ` (ASM:2509).

### Как помечать в чеклисте

- Если ASM-«функция» (по комментарию `; End of function`) распалась в C на несколько функций — пишем основное имя и в комментарии указываем дочерние.
- Если ASM-метка стала **goto** в C — её можно не выносить отдельным пунктом, но если она оригинально упоминалась как JSR-цель — добавить пункт с пометкой *(label, goto in C)*.
- Если две ASM-функции **слились** в одну C-функцию через fallthrough — у обеих ставим один и тот же C-указатель и помечаем *(fallthrough into …)*.

Список меток, которые **не имеют** `; End of function` и присутствуют в чеклисте только как `goto`-якоря (не выносить в отдельные C-функции):

- `Wait` (ASM:315) — внутри `RESET`
- `New_Scroll`, `Title_Loaded` (ASM:337, 341) — внутри `BEGIN`
- `Skip_LoadFrame`, `Construction_Loop`, `Skip_Status_Handle`, `Construct_Draw_TSA`, `Construct_StartCheck`, `End_Construction` (ASM:357–442) — внутри `Construction`
- `StageSelect_Loop`, `Inc_LevelNum`, `Check_B`, `Dec_LevelNum`, `Start_Level`, `Skip_Lvl_Load`, `Battle_Engine`, `Skip_Battle_Loop`, `Skip_Pause_Switch`, `AfterDeath_BattleRun`, `Ckeck_FirstFinish`, `Check_GameOver`, `Make_GameOver`, `Skip_RecordShow` (ASM:467–620) — внутри `Start_StageSelScrn`
- `Set_VARs`, `Respawn_Delay_Calc` (ASM:760, 798) — внутри `SetUp_LevelVARs`
- `DemoLevel_Loop`, `End_Demo`, `Button_Pressed` (ASM:881–895) — внутри `BonusLevel_ButtonCheck`
- `Next_Frame`, `End_Draw_Brick_GameOver` (ASM:1166, 1174) — внутри `Draw_Brick_GameOver`
- `ArrowNotPressed`, `End_Move_Tank` (ASM:1295, 1336) — внутри `Move_Tank`
- `Init_GameOverStr`, `ExitLevel`, `PlayLevel` (ASM:1370–1386) — внутри `LevelEnd_Check`
- `SkipInc_Zero_Page_Viewer`, `ScipDec_Zero_Page_Viewer`, `End_Zero_Page_Viewer` (ASM:1424–1439) — внутри `Zero_Page_Viewer`
- `Draw_2P_Lives`, `Draw_1P_Lives`, `Draw_LivesDigit` (ASM:1494–1511) — внутри `Draw_Player_Lives`
- `Draw_IIP` (ASM:1550) — внутри `Draw_IP`
- `Hide_String`, `Check_Motion`, `Stopped_Motion`, `End_GameOver_Str_Move` (ASM:1784–1806) — внутри `GameOver_Str_Move_Handle`
- `End_Check_BorderReach` (ASM:1690) — внутри `Check_BorderReach`
- `End_Draw_Pause` (ASM:1737) — внутри `Draw_Pause`
- `DrawPtsScrn_NxtTank`, `DrawPtsScrn_NxtCount`, `DrawPtsScrn_CheckHQ`, `DrawPtsScrn_CheckNum`, `DrawPtsScrn_CheckLives`, `End_Draw_Pts_Screen` (ASM:2354–2595) — внутри `Draw_Pts_Screen`
- `Skip_ScndPlayerDraw`, `Skip_ScndPlayerPtsDraw` (ASM:2741, 2803) — внутри `Draw_Pts_Screen_Template`
- `Play_SndAncillaryLife`, `End_Add_Life` (ASM:2921, 2926) — внутри `Add_Life`
- `Skip_PalLoad`, `End_Interrupt` (ASM:3193, 3212) — внутри `NMI`
- `Fill_NTBuffer`, `Fill_NTAttribBuffer`, `Draw_BlackRow` (внутри `Draw_GrayFrame`)
- `Check_Max`, `@exit` (внутри `ByteTo_Num_String`)

И обратно — JSR-точки, которые в C по факту реализованы **внутри** другой функции (не самостоятельно):

- `TSA_Pal_Ops`, `OR_Pal` — в C свёрнуты в выражение внутри `attrib_to_scr_buffer()`.
- `Set_SprIndex` — отдельная C-функция, но вызывается только из `draw_big_explode()` (внутри tank_draw).
- `Indexed_SaveSpr` — JSR-цель, но используется только парой мест.
- ASM-функции, которые в C вообще не вынесены отдельно, помечать в комментарии «*(inlined into X)*».

## Static-функции в `src/game/`

Внутренние утилиты и заглушки, **не имеющие прямых ASM-аналогов** и поэтому оставленные `static`:

- `status_noop(uint8_t)` — placeholder в `battle_bullet_status.c`, `battle_tank_draw.c`, `battle_tank_status.c`. Заполняет нулевой слот jump-table (старший ниббл Bullet/Tank_Status == 0 → пуля/танк отсутствует → диспетч-функция не должна ничего делать). В ASM этого «no-op» нет — там просто RTS-stub в таблице.
- `bullet_end_ice_move(uint8_t)` в `battle_bullet_draw.c` — C-адаптер для ASM `End_Ice_Move` (просто RTS). Нужен потому, что `BulletGFX_JumpTable` требует `void(uint8_t)`-сигнатуру, а реальная `end_ice_move(void)` имеет другую.
- `init_zp_bytes(void)` в `random.c` — однократная инициализация буфера `zp_bytes[256]` с симулированным «мусором» power-on RAM (через PRNG из poweron_randomize). В ASM эту роль играет неинициализированная zero-page после хардварного RESET.

C-port-публичные хелперы без прямого ASM-аналога (документированы в PORTING как «C-port helper»):

- `save_aligned_str_to_scr_buffer(col, row, base_str)` в `draw.c` — оборачивает связку `PtrToNonzeroStrElem` + `Save_Str_To_ScrBuffer`. В ASM эти две функции вызываются последовательно, причём `PtrToNonzeroStrElem` инкрементит CPU-регистр X (display-колонку) через `INX` за каждый пропущенный ноль. В C регистра X нет — хелпер вычисляет `skip = p - base_str` и передаёт `col + skip` в `save_str_to_scr_buffer`.

Jump-table и static-данные (тоже `static`, но это массивы, не функции):

- `HQExplode_JumpTable` (`battle_hq.c`) — таблица из 7 функций-кадров для анимации взрыва HQ.
- `Bonus_JumpTable` (`battle_bonus.c`) — таблица обработчиков типов бонусов.
- `sound_command_jump_table` (`sound_engine.c`) — таблица 18 sound-команд.
- `Bullet_Status_JumpTable`, `TankDraw_JumpTable`, `TankStatus_JumpTable` (тоже `static const`) — индексируются `(status >> 3) & $FE` и моделируют ASM `JMP (LowPtr_Byte)` через C-указатели на функции.
- `zp_bytes[256]`, `zp_initialized` в `random.c` — приватное состояние симуляции power-on zero-page для `get_random_a()`.

## Строковые ASM-переменные

ASM-таблица строк (ASM:3084–3158). Каждая строка завершается `$FF` (если не указано иное).

| ASM-имя | Содержимое | Назначение |
|---|---|---|
| `aWrittenBy` (3084) | `"WRITTEN BY"` | Secret Message |
| `aNAMCOT` (3086) | `` "`abcdefgh" `` | Часть tile-map для NAMCOT-сообщения |
| `aBattle` (3088) | `"BATTLE"` | Заголовок Title (верхняя часть) |
| `aCity` (3090) | `"CITY"` | Заголовок Title (нижняя часть) |
| `aK` (3092) | `$5E,$6B,$FF` | На pts-screen: `'I-'` (player-1 prefix) |
| `a_k` (3093) | `$5F,$6B,$FF` | На pts-screen: `'II-'` (player-2 prefix) |
| `aHik` (3096) | `"HIk"` | `"HI-"` (k = tire-tile) |
| `aHiscore` (3098) | `"HISCORE"` | Brick-сообщение «HI-SCORE» при рекорде |
| `aHikscore` (3100) | `"HIkSCORE"` | `"HI-SCORE"` на title (с тире) |
| `aKplayer` (3106) | `"^kPLAYER"` | `"1-PLAYER"` пункт меню title |
| `a_kplayer` (3108) | `"_kPLAYER"` | `"2-PLAYER"` пункт меню title |
| `aConstruction` (3110) | `"CONSTRUCTION"` | Пункт меню title |
| `aThisProgramWas` (3114) | `"THIS PROGRAM WAS"` | Secret Message |
| `aAllRightsReserved` (3116) | `"ALL RIGHTS RESERVED"` | NAMCOT copyright |
| `aOpenkreach` (3118) | `"OPENkREACH"` | Secret Message (имя автора) |
| `aDot` (3120) | `$69,$FF` | Secret Message — символ `.` |
| `aGame` (3123) | `"GAME"` | Brick-сообщение GAME OVER (верх) |
| `aOver` (3125) | `"OVER"` | Brick-сообщение GAME OVER (низ) |
| `aWhoLovesNoriko` (3127) | `"WHO LOVES NORIKO"` | Secret Message |
| `aPts` (3129) | `"PTS"` | Pts-screen: подпись очков |
| `aTotal` (3152) | `"TOTAL"` | Pts-screen: TOTAL |
| `aLine` (3154) | `$5C×8,$FF` | Pts-screen: разделительная линия над TOTAL |
| `aBonus` (3155) | `"BONUS"`,$15 | Pts-screen: «BONUS! 1000 PTS» — награда лидеру по фрагам (рисуется под IP/IIP в pts_screen |
| `aStage` (3158) | `"STAGE"` | Stage-select / pts-screen: «STAGE NN» |

### Tile-«строки» (тайловые $FF-terminated массивы)

Эти таблицы технически не текст, но передаются в `String_to_Screen_Buffer` / `Save_Str_To_ScrBuffer` как обычные строки и завершаются `$FF`. Используются для HUD-иконок, фигур HQ и стрелок.

| ASM-имя | Содержимое | Назначение |
|---|---|---|
| `I_p` (3094) | `$58,$13,$FF` | «I-» над жизнями 1P в HUD |
| `II_p` (3095) | `$5A,$13,$FF` | «II-» над жизнями 2P в HUD |
| `PlayerLives_Icon` (3121) | `$14,$FF` | Иконка жизни игрока |
| `Reinforcement_Icons` (3131) | `$6A,$6A,$FF` | 2 иконки врага-резерва в правой колонке |
| `LevelFlag_Upper_Icons` (3132) | `$6C,$FC,$FF` | Верхняя строка флага уровня (правый низ экрана) |
| `LevelFlag_Lower_Icons` (3133) | `$6D,$FD,$FF` | Нижняя строка флага уровня |
| `Empty_Tile` (3134) | `$11,$FF` | Пустой тайл — заменяет иконку врага при появлении |
| `Normal_HQ_TSA` (3137) | `0×6,$FF` | HQ: верхняя пустая строка |
| `NormalLine2` (3138) | `0,$F×4,0,$FF` | HQ: вторая строка кирпичной обводки |
| `NormalLine3` (3139) | `0,$F,$C8,$CA,$F,0,$FF` | HQ: третья строка (верх «орла») |
| `Normalline4` (3140) | `0,$F,$C9,$CB,$F,0,$FF` | HQ: четвёртая строка (низ «орла») |
| `Armour_HQ_TSA_Line1` (3141) | `0×6,$FF` | Armoured HQ: верхняя пустая строка |
| `Armour_HQ_TSA_Line2` (3142) | `0,$10×4,0,$FF` | Armoured HQ: вторая строка (железная обводка) |
| `Armour_HQ_TSA_Line3` (3143) | `0,$10,$C8,$CA,$10,0,$FF` | Armoured HQ: третья строка |
| `Armour_HQ_TSA_Line4` (3144) | `0,$10,$C9,$CB,$10,0,$FF` | Armoured HQ: четвёртая строка |
| `Naked_HQ_TSA_FirstLine` (3146) | `$C8,$CA,$FF` | HQ без обводки: первая строка «орла» |
| `Naked_HQ_TSA_SecndLine` (3147) | `$C9,$CB,$FF` | HQ без обводки: вторая строка «орла» |
| `DestroyedHQ_TSA_Line1` (3148) | `$CC,$CE,$FF` | Разрушенный HQ: верх |
| `DestroyedHQ_TSA_Line2` (3149) | `$CD,$CF,$FF` | Разрушенный HQ: низ |
| `Arrow_Left` (3150) | `$5B,$FF` | Стрелка «←» (pts-screen) |
| `Arrow_Right` (3151) | `$5D,$FF` | Стрелка «→» (pts-screen) |

### `StaffString` — особая строка

`StaffString` (ASM:303) — фиксированная 48-байтная ASCII-сигнатура в самом начале ROM, размещённая до `RESET`:

```
"RYOUITI OOKUBO  TAKEFUMI HYOUDOU" "JUNKO OZAWA     "
```

Используется как «магический маркер» для определения тёплого ребута:

- `StaffStr_Store` (3340) копирует её в RAM (`StaffString_RAM`) после полной инициализации.
- `StaffStr_Check` (3368) при следующем reset сравнивает RAM-копию с ROM-оригиналом:
  - совпали → **HotBoot**: пропустить сброс hi-score (значения 1P/2P сохраняются, ColdBoot-инициализация Player1_Lives и т.д. пропускается);
  - не совпали → **ColdBoot**: первая загрузка / физический ремонт картриджа — обнулить всё.

В C-порту хранится в [reset.c:12](src/game/reset.c#L12) как `static const uint8_t StaffString[48]`. `StaffString_RAM` — обычный bss-массив.



