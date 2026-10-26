# 构建与验证记录

日期：2026-10-26。

## 1.0.1 路径编码修复

旧版使用 `getenv()` 取得 Windows 存档目录，再交给 `std::filesystem::path(char*)`。Windows 本地代码页与 MinGW 的窄字符串解释方式可能不一致，导致中文用户名等路径在启动时抛出转换异常。

修复内容：`LOCALAPPDATA` 和 `LUMENFALL_SAVE_DIR` 均由 `GetEnvironmentVariableW` 读取，以 UTF-16 直接构造 Windows 原生路径；不经过窄字符转换。读取存档时的异常也会返回读取失败状态，避免直接退出程序。

本次验证：

- Linux 上原有 238 项逻辑断言，以及新增 30 项路径断言全部通过。
- 新增路径测试覆盖 ASCII、空格、中文、日文、重音字符和 emoji 的路径选择、读写、替换、备份及缺失文件处理。
- Windows 游戏和 Windows 专用路径测试程序均成功交叉编译。Windows 测试还覆盖无自定义目录时的中文 `LOCALAPPDATA` 以及自定义目录的优先级；这些 Windows 专用分支尚未在原生 Windows 运行。
- 新版可执行文件窗口标题为 `LUMENFALL 1.0.1`。原生 Windows 启动验证仍待完成。

实现参考：[Microsoft GetEnvironmentVariableW](https://learn.microsoft.com/en-us/windows/win32/api/processenv/nf-processenv-getenvironmentvariablew)、[GCC 路径编码问题记录](https://gcc.gnu.org/pipermail/gcc-bugs/2023-August/834648.html)。

下方为初版的其余验证记录。

## 已完成

| 项目 | 结果 |
|---|---|
| Linux，GCC 13，C++17，SFML 3.0.2，图形与音频均启用 | 成功编译、链接 |
| Windows x64，MinGW-w64，SFML 3.0.2 静态链接 | 成功交叉编译、链接，生成 `play-windows/Lumenfall.exe` |
| Windows 可执行文件导入检查 | 仅依赖 Windows 系统 DLL，无需另外放置 SFML DLL |
| 随包依赖离线配置 | 使用 `FETCHCONTENT_FULLY_DISCONNECTED=ON` 完成 Windows 工程配置 |
| CTest 逻辑回归 | 5 组测试、238 项断言全部通过 |
| AddressSanitizer + UndefinedBehaviorSanitizer | 同一组逻辑测试通过，未报告内存访问错误或未定义行为 |
| 实际 SFML 图形渲染 | 标题、枢纽、花园、Boss、地图、结局 6 个场景均成功生成截图，已查看布局 |
| 图形启动烟雾测试 | 启用音频初始化的 360 帧运行正常退出 |

## 测试范围

1. **移动**：长短跳、离地宽容、落地前预输入、冲刺能力限制与无敌时间。
2. **地图与能力**：门目标有效、出生点不嵌入实体、不处于危险区；能力门限制；冲刺破障；使用真实逐帧输入走通水道取得冲刺、档案馆取得二段跳、枢纽高门、花园、铸钟厂钟心、长桥与 Boss 入口；花园下方秘密平台可返回主路。
3. **战斗与恢复**：一次挥击只命中同一敌人一次、三连击、治疗消耗、灯笼补给、死亡保留能力与回响。
4. **Boss**：入场触发、战中封门、冲锋/砸地/落雷循环、反击时间、半血转阶段、玩家攻击导致 Boss 死亡、清空残留攻击、触发结局、未完成战斗重置。
5. **存档**：读写往返、备份生成、损坏校验、非法复活点拒绝。

## 验证边界

- 图形运行与截图验证在 Linux 的虚拟 X11 / 软件 OpenGL 环境完成。
- Windows 文件已成功交叉编译，**尚未在原生 Windows 桌面实玩验证**；MSVC 和 macOS 构建入口也尚未在相应系统运行验证。
- 地图路线测试关闭敌人干扰，专门验证实际物理可达性。Boss 测试包含可控位置、无敌与时间夹具，用于验证状态和伤害流程；不能替代真人操作难度评测。
- 无音频播放设备的环境仍可进入游戏；本次没有进行扬声器输出或音乐听感验收。

## 复现

```bash
cmake -S . -B build-core -DLUMEN_BUILD_GAME=OFF
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

游戏本体构建和截图模式见根目录 `README.md`。`preview` 中的 PNG 是本工程运行产生的画面。
