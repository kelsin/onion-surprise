# Changelog

## 0.1.0 (2026-11-19)


### :video_game: Features

* add core paths, games loader and launch logic with host tests ([78b9b32](https://github.com/kelsin/onion-surprise/commit/78b9b3207899fa673a42675735387a2996374ff5))
* **icons:** add one-colour Surprise icons for TechDweeb themes ([6cf80a5](https://github.com/kelsin/onion-surprise/commit/6cf80a5602bd0d328f91804dffe9251038b8facc))
* packaging, deploy and surprise.json helpers, icon, docs ([ba45023](https://github.com/kelsin/onion-surprise/commit/ba45023f6228b2ec8b555a87f6fbe34df8ca4119))
* theme loader, list UI with box art, and input loop ([6efc2a7](https://github.com/kelsin/onion-surprise/commit/6efc2a787947f80edd6a74048e6bfb7ff17d4ae5))


### :bug: Bug Fixes

* compact config.json, package.sh stdout, deploy without log.txt, simpler test assert ([3621142](https://github.com/kelsin/onion-surprise/commit/3621142666d5ff74be3206ce8dfb37c336afe74a))
* deploy over a single ssh connection with tar ([767ac0c](https://github.com/kelsin/onion-surprise/commit/767ac0cff8b6c824bc6d26edd3c07b4016b6e616))
* mirror C value rules and line limit in surprise.json generator ([fece1a3](https://github.com/kelsin/onion-surprise/commit/fece1a364911d36b1354151dc11dffa8cc1bab5c))
* section font size falls back to title like colour and font ([2bb9069](https://github.com/kelsin/onion-surprise/commit/2bb9069e5229e949205a299bd3683e865ed8d884))
* treat empty imgpath as absent ([ad73bdb](https://github.com/kelsin/onion-surprise/commit/ad73bdb116a5d847291d43583ffee099a7cb8dd4))
* **ui:** draw theme images and box art upright on device ([b536386](https://github.com/kelsin/onion-surprise/commit/b53638652052d70c03519a1edab0481ef86307d6))


### :broom: Chores

* ignore macOS, Python and editor clutter ([a93e296](https://github.com/kelsin/onion-surprise/commit/a93e296fa91683bea058551836c5a5326d93d0e0))


### :book: Documentation

* add CLAUDE.md with build steps and device rules ([f2364ed](https://github.com/kelsin/onion-surprise/commit/f2364edef03bd4a41afa073f2bab5e5e608082b6))
* extend device testing checklist ([eadef4e](https://github.com/kelsin/onion-surprise/commit/eadef4e14495eed8f7e8ed07b8dbed62b5bef84f))
* research notes and implementation plan ([3f1509e](https://github.com/kelsin/onion-surprise/commit/3f1509e94c16b5481575973dc5c36796af92f43a))
* **research:** record on-device findings for images, recents and icons ([a2e5ba0](https://github.com/kelsin/onion-surprise/commit/a2e5ba0e40150c5ebfc173f1f6d1081090b4d42c))


### :traffic_light: Tests

* fixture card and headless preview screenshots ([3c4236f](https://github.com/kelsin/onion-surprise/commit/3c4236f476b47347e5de85f0b195ca2b0f6f8da6))


### :package: Build System

* add toolchain image, Makefile, build script and SDL placeholder ([f01d177](https://github.com/kelsin/onion-surprise/commit/f01d177e6eb8a53cdf098fe3550536036453d76e))
* create output dir before linking ([e07b2c5](https://github.com/kelsin/onion-surprise/commit/e07b2c5410b5ab7f054a711f33f3d99278607b4c))
* **package:** install the Surprise icon as Onion's default app icon ([ae08dff](https://github.com/kelsin/onion-surprise/commit/ae08dff077d00afe773cbaea5bce2b0083834723))


### :gear: Continuous Integration

* release with release-please and publish zips to storage ([b846894](https://github.com/kelsin/onion-surprise/commit/b846894277bcaa1a1295f69e32d1893e5c52617e))
