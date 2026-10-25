# Magician's Saga — pesquisa para port nativo

Objetivo: port fiel para Windows/Linux, com núcleo independente de plataforma e avaliação posterior de Nintendo 3DS. As regras estão em [AGENTS.md](AGENTS.md).

Estado: **M0 em andamento; M1 parcial**. A arquitetura observada envolve Unity/IL2CPP e integração Android. Extração de assets, leitores C++17/CMake e visualizador técnico SDL3 foram validados em Windows. Ainda não existe jogo reconstruído; Linux, comportamento runtime e viabilidade 3DS continuam pendentes.

## Navegação

- [Índice dos relatórios por capítulos](docs/README.md)
- [Plano M0–M6 e mapa do código](docs/plan.md)
- [Mudanças realizadas](CHANGELOG.md)
- [Progresso atual](docs/progress.md) e [fluxo Git](CONTRIBUTING.md)

## Estrutura

```text
docs/                  Evidências, arquitetura, perguntas e progresso
tools/                 Inventário e preparação local das ferramentas
tests/                 Testes de formatos e contratos; comparação runtime pendente
src/core/gameplay/     Regras de combate, recompensas e fluxo de eventos
src/core/animation/    Transições e codecs de animação
src/core/ecs/          Componentes e filtros
src/core/persistence/  Codec de payload de scripts
cmake/                 Configuração comum dos módulos C++17
src/assets/            Leitores portáteis de contêineres, OPT e SEB
src/platform/desktop/  Ferramentas nativas de inventário e visualização
src/platform/n3ds/     Reservado para estudo de viabilidade
research/local/        Evidências locais, ignoradas pelo Git
data/                  Assets extraídos da cópia local, ignorados pelo Git
.tools/                Ferramentas locais, ignoradas pelo Git
```

## Reproduzir o inventário

Python 3.12 foi usado; não há dependências Python externas.

```powershell
python tools/inspect_apk.py "Magician's+Saga_1.4.0_APKPure.apk" --output research/local/apk-inventory.json
```

O script lê o APK sem executá-lo ou extrair seus assets. O relatório não valida a assinatura, não resolve referências Android de recursos e não demonstra funcionamento offline.

Veja [inventário](docs/01-apk-inventory.md), [arquitetura](docs/02-architecture.md), [pendências](docs/03-open-questions.md), [ferramentas](docs/tooling.md) e [progresso](docs/progress.md).

Investigações detalhadas: [boot Android](docs/04-android-startup.md), [catálogo IL2CPP](docs/05-il2cpp-metadata.md) e [bootstrap nativo ARM64](docs/06-native-bootstrap.md).

Não incluir APK, assets, saves pessoais nem código proprietário decompilado no controle de versão. Ainda não há build do jogo; há um leitor nativo de assets para validar a futura camada portátil.

Também há um módulo C++17 de transições de animação por entidade, reconstruído do assembly e testado no Windows. Ele preserva troca, interrupção e restauração de animação, mas ainda não está integrado a um loop ou cena. Evidência, contrato e limites em [30-animation-transitions.md](docs/30-animation-transitions.md); o build abaixo executa também seus testes.

## Leitor nativo de assets

```powershell
tools\build-desktop.cmd
build\desktop\asset_inspect.exe data\extracted\textassets\00_hat.bin
```

O build usa a toolchain Visual Studio instalada, configura CMake e executa CTest. O executável emite o índice JSON de um contêiner já decodificado. Veja [contrato, teste diferencial e comandos Linux ainda não executados](docs/15-native-assets.md).

## Extrair os recursos da sua cópia

Para o APK 1.4.0 de hash registrado no inventário:

```powershell
uv run tools/extract_assets.py "Magician's+Saga_1.4.0_APKPure.apk" --out data/extracted
uv run tools/extract_assets.py --check --out data/extracted
```

Extrai 38 TextAssets e 1.963 arquivos internos, incluindo 973 PNGs. O script obtém a chave do próprio APK e valida reconstrução exata dos 30 contêineres. Nomes originais e hashes ficam em `data/extracted/manifest.json`; saídas não devem ser distribuídas. Imagens, tabelas e animações extraídas ainda não constituem um jogo executável. Objetos Unity de áudio/fontes/texturas não estão incluídos nesta etapa. Veja [formato e validação](docs/12-asset-extraction.md).

Para incluir também os objetos Unity (68 texturas em PNG com dados codificados originais, 23 pares FSB/WAV e cinco fontes), use um destino separado:

```powershell
uv run tools/extract_assets.py "Magician's+Saga_1.4.0_APKPure.apk" --out data/full --unity-media
uv run tools/extract_assets.py --check --out data/full
```

Cinco texturas vazias são registradas no manifesto. Fontes são preservadas, inclusive inconsistências do original. Esta conversão foi validada em Windows; não equivale à reprodução fiel de áudio/fontes no port. Veja [mídia Unity](docs/14-unity-media.md).

## Visualizador técnico opcional

O núcleo também contém módulos de [transições de animação](docs/30-animation-transitions.md), [armazenamento de componentes](docs/35-component-slots.md), [predicados de filtros](docs/39-generic-collections.md), [codecs de payloads](docs/43-animation-payload-codec.md), [fluxo de eventos](docs/82-native-event-flow.md) e [efeitos de progressão](docs/83-event-progression-effects.md), recuperados por análise estática. O build padrão executa dezenove testes de contrato, incluindo integração de eventos e progressão; esses módulos ainda não constituem um ECS integrado ou gameplay completo.

```powershell
tools\build-viewer.cmd
build\viewer\asset_viewer.exe data\full\members\31_title\0006_title.png
```

Abre o PNG em uma janela redimensionável; Escape fecha. O build opcional baixa fontes SDL3/stb fixadas por versão/hash para `build/`. O leitor padrão continua sem SDL. Capturas headless foram comparadas pixel a pixel; não há cena/menu do jogo implementado. Veja [comandos de captura e limites](docs/19-asset-viewer.md).
