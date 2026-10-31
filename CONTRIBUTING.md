# Contributing

Please include the release version, Windows version, selected language, screen resolution, Steam launch method and steps to reproduce. For UI bugs, attach a gameplay screenshot and describe which control was selected. For networking, distinguish lobby failures, disconnections and delayed commands, and include relevant timing-log excerpts after reviewing them for personal information.

Do not upload the game executable, proprietary game assets, saves containing personal information, Steamworks SDK headers, credentials or join codes.

Keep graphics and input rectangles aligned. Preserve the original simulation timing unless a multiplayer test proves a synchronized change works on both peers. Do not drop reliable orders to hide latency. Add meaningful regression checks for rendering bounds, installation/restore and protocol changes.

Run the checks in BUILDING.md. Document what was tested locally and what still needs two-PC or visual verification. Changes to the original project code are contributed under MIT; keep third-party licenses and attribution intact.
