# JSON2Struct 插件模块划分

## 思路：编辑器模块名 JSON2StructEditor；运行时模块名 JSON2StructRuntime

- **JSON2StructEditor（Editor）** = **带界面的主模块**  
  工具栏/菜单、JSON2Struct 面板、URL 输入、请求、JSON 编辑、生成/合并 Structure 等。仅编辑器加载，依赖运行时模块 JSON2StructRuntime。

- **JSON2StructRuntime（Runtime）** = **运行时模块**  
  无 UI；Rest 请求返回 JSON 并自动解析为 USTRUCT。对外仅暴露 `#include "JSON2Struct.h"`，根结构体及依赖由模块内部总览头提供。生成文件默认写到本模块 `Public/Generated/`。

## 依赖关系

```
JSON2StructEditor (Editor)  ——依赖——>  JSON2StructRuntime (Runtime)
```

## 目录与职责

| 模块 | 路径 | 职责 |
|------|------|------|
| JSON2StructEditor | `Source/JSON2StructEditor/` | 面板 UI、Style、Commands、Settings、生成逻辑；仅编辑器 |
| JSON2StructRuntime | `Source/JSON2StructRuntime/` | Rest、BPLibrary、Generated 总览头（根结构体及依赖）；无界面，运行时 |

生成的结构体默认写入 `Source/JSON2StructRuntime/Public/Generated/`，由编辑器模块在生成时写入，运行时模块引用。
