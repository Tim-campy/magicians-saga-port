# Perguntas em aberto

Data: 2026-10-25. M0 permanece parcial; nenhuma execução do original foi observada.

## Questões já reduzidas

JADX funciona e sustentou a conferência DEX/smali do bootstrap. Há registro JNI NativeLoader, catálogo IL2CPP v31 e endereços ARM64 dos callbacks main.Main. A cena registrada e os scripts foram ligados por referências, não apenas nomes. O split Unity foi validado; 23 streams de áudio referenciados estão presentes no APK. A extração de recursos cifrados foi explicitamente autorizada pelo usuário e produziu 38 TextAssets, 1.963 membros e 973 PNGs validados. Esses resultados não estabelecem regras, apresentação fiel, completude ou funcionamento offline.

Também há parsers estruturais SEB/OPT com round trips integrais, exportação dos objetos Unity de mídia, leitores nativos C++17 e visualizador técnico com captura RGBA validada. Os formatos de disco comprovados ainda não equivalem à composição de cenas e animações do jogo.

Evidências: [07](07-il2cpp-code-map.md), [08](08-smali-validation.md), [09](09-native-callbacks.md), [10](10-unity-assets.md), [12](12-asset-extraction.md), [13](13-animation-formats.md), [14](14-unity-media.md), [15](15-native-assets.md), [18](18-optimized-textures.md), [19](19-asset-viewer.md), [22](22-unity-lifecycle.md) e manifesto local `data/full/manifest.json`.

## Questões ativas

| Prioridade | Questão | Critério de saída |
| --- | --- | --- |
| 1 | Como interpretar `.seb`/`.inf` e tabelas extraídas? | Layout sustentado por leitores nativos, limites e referências verificados; teste de round trip sem aproximação |
| 1 | Todos os dados de gameplay estão presentes? | Cadeias de carregamento/download e observação controlada; ausência de `.obb` ou URL não prova completude |
| 1 | Rede, billing, anúncios e Firebase são obrigatórios? | Call sites/condições e testes autorizados que distingam dependências opcionais, sem contornar controles |
| 1 | Como a cena ativa o framework e o primeiro formulário? | Cadeia de ativação e captura de boot; cena e endereços já estão localizados |
| 1 | Quais são as regras de combate, IA, progressão e economia? | Campos, condições e fórmulas comprovados e cenários comparáveis; nomes de métodos não bastam |
| 2 | Qual o timing efetivo? | Valores iniciais e condições de GetTargetFps/vSyncCount/targetFrameRate; separar simulação, apresentação e input |
| 2 | Como compor a UI e animar recursos? | Slot virtual OnDraw, atlas, coordenadas, sequência e duração de frames verificados contra o original |
| 2 | Como reproduzir áudio e fontes? | Formatos decodificados, glifos/gatilhos/volume compreendidos e prova visual/auditiva |
| 2 | Como saves e RNG funcionam? | Caminhos, schema, condições de save, algoritmo, seeds e cenários determinísticos |
| 2 | Como resolver divergências do decompilador? | Assembly e limites conferidos; pseudocódigo contaminado de OnUpdate não é base de implementação |
| 3 | O port cabe no 3DS? | Benchmark real de memória, CPU, texturas, telas e controles em M6 |

## Próxima investigação delimitada

Resolver as dimensões lógicas completas de Image e a cadeia TexId/ResourceManager/INF que seleciona PNG/OPT. Os parsers estruturais já foram verificados; a apresentação fiel depende das transformações e do contexto de carregamento, não apenas de recortar imagens. A autorização de extração resolveu a pausa anterior; não estabelece equivalência runtime.

Observação de boot, input, timing e áudio fica adiada: o usuário informou indisponibilidade de testes dinâmicos e pediu continuidade estática. Após três tentativas de pseudocódigo inadequado, continuar a investigação de OnUpdate por funções pequenas/assembly, conforme relatório 09. Os filtros de Update/OnGUI já foram aprofundados em [24](24-frame-conditions.md); catálogos e referências pendentes em [25](25-resource-catalog-audit.md). Ainda há avanço estático útil.

As duas sobrecargas `AppData.ResourceReDownload` (34449/34450, ELF `0x1111970/0x1111974`) têm FDE de quatro bytes e word `0xd65f03c0` (`RET X30`). Conferência direta dos bytes, offsets e SHA-256 em `build/research/resource-redownload-stubs.json`. São corpos vazios nesta biblioteca específica; isso não exclui download por outros métodos, serviços ou recursos externos, nem demonstra funcionamento offline.

## Aceitação de M0

- [x] APK localizado; hash, identidade, versão, DEX, ABIs, ELF e assets inventariados.
- [x] Manifesto comparado com resultado independente JADX.
- [x] Bootstrap Android/JNI e ramos de foco/reagendamento documentados estaticamente.
- [x] Metadata e mapa estrutural IL2CPP produzidos; callbacks com endereços e intervalos FDE.
- [x] Cena, scripts e recursos Unity catalogados; split e streams de áudio validados.
- [x] Extração autorizada de TextAssets/contêineres validada, sem iniciar port de gameplay.
- [ ] Startup completo, loop, vídeo, áudio, input, persistência e carregamento suficientemente mapeados para comparação.
- [ ] Regras e responsabilidades de gameplay confirmadas além de nomes e chamadas de framework.
- [ ] Schemas de conteúdo, completude e necessidade de serviços/downloads estabelecidos.
- [ ] Original observado em cenários controlados e parecer de viabilidade revisto.

Os itens estáticos concluídos não implicam fidelidade funcional. M0 continua aberto.
