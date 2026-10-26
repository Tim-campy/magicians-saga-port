# M1 — áudio, fontes e texturas Unity

Investigação local em 2026-10-25. APK SHA-256:
`d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`.
Estado: extração e validação estrutural confirmadas; apresentação e reprodução
equivalentes ao Android ainda não verificadas.

## Evidências reproduzíveis

- Inventário de origem: `research/local/unity-assets.json`, objetos identificados
  por `file` e `path_id`; referências de áudio em `stream_references`.
- Protótipo local: `build/research/media/inspect_media.py`.
- Resultado local: `build/research/media/manifest.json`, com SHA-256 dos streams,
  WAV, PCM e fontes, dados ffprobe e checksums das tabelas OpenType.
- Comando do protótipo, na raiz: `python build/research/media/inspect_media.py`.
  Ele usa as dependências existentes em `.tools/unitypy-libs` e ffprobe no PATH.
- Duas execuções finais produziram manifesto idêntico, SHA-256
  `4d53f1ee279bd0d76f7db1364bf08e81ffd24ddce4aef518d54271823b0d57f8`.
  O protótipo e suas saídas são locais/ignorados; a incorporação ao extrator
  público é uma etapa separada.

Nenhum arquivo do APK foi modificado. Nenhum asset foi obtido externamente.
Não foi usado processo Java/Ghidra nesta investigação.

## Áudio: 23 streams disponíveis e decodificados

Os 23 `AudioClip` apontam para intervalos presentes em `.resource` no próprio
APK. Cada intervalo começa por `FSB5`, possui versão 1, um subsom e codec
`0x0f`. O decoder de referência do [vgmstream](https://github.com/vgmstream/vgmstream/blob/master/src/meta/fsb5.c)
identifica esse código como Vorbis e descreve o cabeçalho de 60 bytes, contagem
de frames e chunks adicionais. A leitura limitada desses campos confirmou
tamanhos, frames e limites em todos os streams; nenhum contém chunk de loop
tipo 3. Isso não determina se o jogo solicita repetição em sua API de áudio.

Fluxo testado:

1. Ler `m_Resource.m_Source`, `m_Offset` e `m_Size`, resolver o membro ZIP pelo
   inventário e conferir os limites antes do recorte.
2. Preservar os bytes FSB completos, com tamanho e hash.
3. Chamar `fmod_toolkit.raw_to_wav(data, name, channels, frequency,
   convert_pcm_float=False)`; exigir exatamente uma saída neste APK.
4. Validar RIFF/WAVE, tamanho declarado, PCM, canais, taxa e frames com `wave`
   da biblioteca padrão e confirmar o formato com ffprobe.

| Medida | Resultado observado |
| --- | --- |
| Streams FSB | 23; 3.784.096 bytes no total |
| WAV | 23; 74.347.948 bytes no total |
| Formato de todos os WAV | PCM signed 16-bit little-endian, 2 canais, 44.100 Hz |
| Frames | Exatamente iguais à contagem de cada cabeçalho FSB |
| Maior diferença de duração frente ao float Unity | 0,0000032393 segundos |
| Chunks de loop FSB | Nenhum nos 23 streams examinados |

Exemplos de âncoras: `bgm_main_02` está no objeto `path_id=1` do arquivo
`assets/bin/Data/0c93f539f36de7f4aa3e0fa5425ca908`; o stream correspondente
tem 748.064 bytes. `se_button` está em
`assets/bin/Data/7e25cb81452e439499dd639e2d39dba9`, `path_id=1`, e produz
31.353 frames.

Ferramentas existentes utilizadas: UnityPy 1.25.4, fmod_toolkit 0.1.3,
pyfmodex 0.7.2, DLL FMOD Windows x64 incluída nessa dependência e ffprobe
7.1.1. O [projeto fmod_toolkit](https://github.com/K0lb3/fmod_toolkit)
documenta essa rota FSB→WAV; ela também é a rota usada por
`UnityPy/export/AudioClipConverter.py` na instalação examinada.

FFprobe/FFmpeg 7.1.1 não aceitou diretamente `se_button.fsb` e respondeu
`Not yet implemented in FFmpeg, patches welcome`. Isso não afetou a conversão
via FMOD. Não foi instalado nenhum decoder adicional.

Limites: não houve comparação acústica, mixagem, volume, pitch, fades,
latência, transições ou loop contra o Android. O WAV preserva o resultado
PCM deste decoder, não permite recompor um stream Vorbis byte a byte. Os FSB
originais foram preservados. Apenas Windows foi testado; a existência de
dependências multiplataforma não valida a mesma conversão em Linux/3DS.
O port nativo não precisa carregar FMOD se consumir os WAV produzidos durante
a extração. O volume de PCM exige política de streaming/memória antes do 3DS.

## Fontes: cinco TTF preservadas

API comprovada: `bytes(obj.read_typetree()['m_FontData'])`. Todos os cinco
payloads têm assinatura TrueType `00 01 00 00`; não são bitmaps. A extração
preserva os bytes integralmente. Os diretórios e intervalos de tabelas foram
checados segundo a [especificação OpenType](https://learn.microsoft.com/en-us/typography/opentype/spec/otff),
incluindo o tratamento de `head.checkSumAdjustment` no checksum da tabela.

| Objeto | Bytes | Nome reportado por FreeType | Checksums |
| --- | ---: | --- | --- |
| `default-th` | 33.872 | Noto Sans Thai / Regular | Todos corretos |
| `default-en-switch` | 2.072.612 | M+ 1p medium / Regular | Todos corretos |
| `default-hi` | 86.736 | Kairo Hindi / Thin | Todos corretos |
| `default-ja` | 1.920.716 | m_plus / Regular | Todos corretos |
| `LegacyRuntime` | 50.788 | Liberation Sans / Regular | Divergências originais descritas abaixo |

As quatro fontes de conteúdo têm checksum global `0xb1b0afba` e todos os
checksums de tabela válidos. `LegacyRuntime`, objeto `path_id=10102` em
`assets/bin/Data/unity default resources`, tem checksum global `0xb249af98`
e divergências nas tabelas `OS/2` e `hhea`. Esses bytes já estão assim no APK;
foram preservados, sem correção silenciosa. Todas as cinco fontes abriram com
FreeType por `PIL.ImageFont.truetype` e renderizaram uma amostra local.

Os previews estão em `build/research/media/fonts/*.png`. O preview japonês
foi inspecionado visualmente e contém `魔法使い 123 ABC` legível. Strings do
protótipo e manifesto estão em UTF-8. Abrir uma fonte e renderizar essa amostra
não prova cobertura de todos os glifos, fallback, kerning, shaping de tailandês
ou hindi, nem equivalência de métricas/layout ao Unity. São testes pendentes.

## Texturas — evidência paralela da tarefa principal

A tarefa principal registrou `research/local/unity-texture-export.json` e
`build/research/unity-textures`: 73 objetos Texture2D, dos quais 68 com imagem
exportável via `texture.image.convert('RGBA')` e round trip PNG com pixels
idênticos. Os cinco objetos restantes são texturas de fontes com dimensões
zero, payload vazio e stream de tamanho zero; não devem gerar imagens
inventadas. A validação de renderização em jogo permanece pendente.

## Próxima etapa

Integração concluída em `tools/extract_assets.py --unity-media`: 187 saídas adicionais
(68 PNG + 68 buffers de textura originais, 23 FSB + 23 WAV, cinco fontes) e cinco
texturas vazias registradas. Execução integrada em `data/full` e `--check` passaram.
Hashes dos 23 WAV e cinco fontes coincidem com o protótipo independente.

```powershell
uv run tools/extract_assets.py "Magician's+Saga_1.4.0_APKPure.apk" --out data/full --unity-media
uv run tools/extract_assets.py --check --out data/full
```

Próximo: testar um sprite, a fonte
selecionada pelo jogo e um efeito sonoro em um protótipo desktop, com captura
comparável ao Android. Não marcar fidelidade audiovisual como concluída apenas
pela leitura bem-sucedida dos formatos.
