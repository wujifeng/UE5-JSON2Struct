# Contributing

感谢你对 `JSON2Struct` 插件的关注。

## 提交范围

- 本插件按独立目录协作，提交范围建议仅包含：`Plugins/JSON2Struct`
- 请避免在同一 PR 中混入项目级无关改动

## 提交前建议

- 确保改动围绕 `JSON2Struct` 插件目标
- 确认目标引擎版本在支持范围：`UE 5.1.1 - 5.7.4`
- 保持模块边界清晰：Editor 逻辑不进入 Runtime
- 新增文件放入正确目录：`Public/` 或 `Private/`
- 本地至少完成一次编译验证

## 提交流程

1. Fork / 新建分支（建议：`feature/*`、`fix/*`）
2. 提交清晰、聚焦的 Commit
3. 发起 Pull Request，说明改动背景、方案与验证方式（并确认改动范围仅为插件目录）
4. 等待 Review 并根据反馈迭代

## 代码风格

- 跟随 Unreal Engine C++ 约定和现有代码风格
- 类型/文件名使用 PascalCase，局部变量使用 camelCase
- 用户可见字符串优先使用 `LOCTEXT` / `NSLOCTEXT`

## Issue 与讨论

- Bug：请附复现步骤、日志、引擎版本、平台信息
- 功能建议：请说明使用场景与预期收益

