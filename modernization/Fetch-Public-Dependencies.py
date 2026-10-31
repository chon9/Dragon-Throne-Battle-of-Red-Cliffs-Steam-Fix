"""Download hash-pinned public build inputs. Steamworks must be supplied separately."""
from pathlib import Path
import hashlib
import io
import urllib.request
import zipfile

ROOT=Path(__file__).resolve().parent
def fetch(url,expected):
    with urllib.request.urlopen(url) as response:data=response.read()
    if hashlib.sha256(data).hexdigest()!=expected:raise ValueError('Download hash mismatch: '+url)
    return data

font=fetch('https://raw.githubusercontent.com/notofonts/noto-cjk/f8d157532fbfaeda587e826d4cd5b21a49186f7c/Sans/OTF/TraditionalChinese/NotoSansCJKtc-Regular.otf',
           'dce08bd4fd91aa8aa76ed8fea4b694c2dfb8550f67871e326843212ddbeb88b4')
dest=ROOT/'vendor/fonts/NotoSansCJKtc-Regular.otf';dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(font)
archive=fetch('https://github.com/FunkyFr3sh/cnc-ddraw/releases/download/v7.1.0.0/cnc-ddraw.zip',
              '0b13ab89a64c9918189b1dadd449ef6ed3cb3b7b19cabd96d8adbd95505bb908')
with zipfile.ZipFile(io.BytesIO(archive)) as z:
    data=z.read('ddraw.dll')
    assert hashlib.sha256(data).hexdigest()=='85e0f7d530dfda134793a57cb3e76b0287dcc96892ee57162dd68f47283b03a9'
    dest=ROOT/'renderers/cnc-ddraw/ddraw.dll';dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data)
print('Verified Noto and cnc-ddraw. Supply a licensed Steamworks SDK separately; see BUILDING.md.')
