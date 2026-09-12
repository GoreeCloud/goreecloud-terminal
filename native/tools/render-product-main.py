#!/usr/bin/env python3

from __future__ import annotations

import pathlib
import re
import sys

OLD_APP_ID = '#define GOREECLOUD_TERMINAL_APP_ID "com.goreecloud.Terminal.Native"'
OLD_RESOURCE = '#define GLAZE_CSS_RESOURCE "/com/goreecloud/Terminal/Native/glaze-ui.css"'
OLD_VERSION = '"version", "0.1.0-dev",'
VERSION_PATTERN = re.compile(r"^[0-9]+\.[0-9]+\.[0-9]+(?:-[0-9A-Za-z.-]+)?$")


def replace_exact_once(source: str, old: str, new: str) -> str:
    count = source.count(old)
    if count != 1:
        raise SystemExit(
            f"identity render refused: expected exactly one marker {old!r}, found {count}"
        )
    return source.replace(old, new, 1)


def main() -> int:
    if len(sys.argv) != 6:
        raise SystemExit(
            "usage: render-product-main.py INPUT OUTPUT APP_ID RESOURCE_PREFIX PRODUCT_VERSION"
        )

    input_path = pathlib.Path(sys.argv[1])
    output_path = pathlib.Path(sys.argv[2])
    app_id = sys.argv[3]
    resource_prefix = sys.argv[4].rstrip("/")
    product_version = sys.argv[5]

    if app_id not in {"com.goreecloud.Terminal", "com.goreecloud.Terminal.Devel"}:
        raise SystemExit(f"unsupported GoreeCloud Terminal application ID: {app_id}")
    if resource_prefix != "/com/goreecloud/Terminal":
        raise SystemExit(f"unsupported GoreeCloud Terminal resource prefix: {resource_prefix}")
    if VERSION_PATTERN.fullmatch(product_version) is None:
        raise SystemExit(f"unsupported GoreeCloud Terminal product version: {product_version}")

    source = input_path.read_text(encoding="utf-8")
    source = replace_exact_once(
        source,
        OLD_APP_ID,
        f'#define GOREECLOUD_TERMINAL_APP_ID "{app_id}"',
    )
    source = replace_exact_once(
        source,
        OLD_RESOURCE,
        f'#define GLAZE_CSS_RESOURCE "{resource_prefix}/glaze-ui.css"',
    )
    source = replace_exact_once(
        source,
        OLD_VERSION,
        f'"version", "{product_version}",',
    )

    if "com.goreecloud.Terminal.Native" in source:
        raise SystemExit("identity render refused: legacy .Native identity remains")

    output_path.write_text(source, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
