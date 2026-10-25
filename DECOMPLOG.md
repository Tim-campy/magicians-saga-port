# Índice de engenharia reversa

- `docs/97-font-texture-delegate.md`: delegate15013 liga métricas a reflexão de editor ou CallStatic<byte[]> Java; seleção efetiva de useFontTexture_ pendente.

- `docs/96-text-lines-and-font-metrics.md`: formação de linhas e Font.StringWidthF ligados às rotas GUIStyle/FontTexture; seleção efetiva/backend alternativo pendentes.

- `docs/95-native-text-parameters.md`: limpeza externa relança erro; parâmetros decimal/hex/RGB portados com host explícito e Windows/CTest18/18. Parser/fonte pendentes.

- `docs/94-text-parameter-exceptions.md`: wrappers ParseInt e handlers locais decimal/hex ligados via LSDA; auditor de255 callsites e dois testes Python; recuperação externa pendente.

- `docs/93-presentation-markup-inventory.md`: inventário lexical das quatro tabelas,19 tags originais e tradução anterior ao parser; sete testes Python. Parser visual pendente.

- `docs/92-native-text-reveal.md`: contagem UTF-16/gráficos e estado temporal portados; Windows/CTest17/17; parser, desenho e integração completa pendentes.

- `docs/91-text-layout-timing.md`: velocidade por idioma, relógio/TotalFrames, campos temporais e distinção Reset/SetText; length dos blocos ainda pendente.

- `docs/90-talk-page-advance.md`: switch do tipo0, Flush versus avanço, continuação e contador de encerramento confirmados estaticamente; temporização e produtores pendentes.

- `docs/89-native-presentation-forms.md`: dados iniciais de fala/evento portados e integrados ao executor em teste; Windows/CTest16/16. UI e catálogo real pendentes.

- `docs/88-native-presentation-arguments.md`: substituição de argumentos portadas; corrigida condição SanSanRemove do relatório85, Windows/CTest15/15.

- `docs/87-native-presentation-split.md`: trim UTF-16<=0x20 e splits de formulário portados; casos de vazio e fronteiras testados, Windows/CTest14/14.

- `docs/86-talk-catalog-producer.md`: BootForm constrói talks_ por Join(TAB), delegate/genéricos e desvio conferidos; tradução/trim continuam pendentes.

- `docs/85-presentation-text-preparation.md`: preparação de falas/eventos e parser de tabelas;46/24 registros por idioma, cinco testes Python. Produtor de talks_/tradução/trim pendentes.

- `docs/84-stamina-and-event-dispatch.md`: limite de stamina, aritmética e routing8/12; dispatcher dos opcodes conhecidos integrado/testado, UI pendente, Windows/CTest13/13.

- `docs/83-event-progression-effects.md`: efeitos1/2/4/15, argumentos e condição de cristais; integração com executor testada, Windows/CTest12/12.

- `docs/82-native-event-flow.md`: executor e fila de contadores em C++17, com host explícito e Windows/CTest11/11; integração de efeitos/guardas pendente.
- `docs/81-delay-origin-and-script-identity.md`: identidade por referência e auditoria das37 esperas; produtor de SubForm.delayEvent_ ainda não localizado.
- `docs/80-project-restructure.md`: módulos, Git e índice temático; não altera regras do jogo.
- `docs/79-gameplay-event-update-guards.md`: retornos antecipados antes da fila e limites da associação ao formulário.

- `docs/78-delay-event-resumption.md`: retomada na seleção/fechamento de formulário, ordem da fila e slots distintos.

- `docs/77-event-opcodes-and-delay.md`: opcodes usados, dispatcher e agendamento especial de DelayEvent.

- `docs/76-event-table-tokenization.md`: tokenizador e ferramenta de leitura dos228 eventos/208 linhas de execução.

- `docs/75-table-encoding-and-line-endings.md`: UTF-8 confirmado, normalização por versão Unity e auditoria de44 textos.

- `docs/74-game-data-text-source.md`: Event.txt ligado ao JarInflater e ao filtro de linhas; encoding/delegate aprofundados nos relatórios seguintes.

- `docs/73-game-data-id-indexing.md`: LoadData indexa por maior ID+1, preservando lacunas; modelo local229 posições.

- `docs/72-return-event-dispatch-and-data-gap.md`: contrato do executor e discrepância entre índice1133 e tabelas extraídas; loader pendente.

- `docs/71-town-return-handler.md`: inscrição de QuestResult, evento1133, atualização do score e flag de retorno.

- `docs/70-treasure-inventory-grants.md`: descoberta, recrutamento e estoque com teto1 portado/testado10/10.

- `docs/69-member-finish-and-return-payload.md`: cópia de progressão ao cadastro, limpeza dos membros e QuestResult no evento31.

- `docs/68-stage-exits-and-result-guards.md`: estados de vitória/falha/saída, guarda de debug e callback do retorno.

- `docs/67-quest-result-reward-grant.md`: bônus dos tesouros, entrega de moedas/EXP, custos de nível e pendências de saída.

- `docs/66-drop-collection-and-stage-rewards.md`: coleta animada/direta e acumulador da fase portado; build/CTest10/10.

- `docs/65-defeat-and-reward-order.md`: ordem de derrota, EXP, contadores e seleção de tesouro por primeiro sucesso; coleta aprofundada em66.

- `docs/64-combat-easing-factors.md`: fatores de dano float32, 148 pontos verificados e build/CTest9/9.

- `docs/63-damage-event-and-quest-record.md`: inscrição de OnDamage, total de dano e recordes entre quests.

- `docs/62-damage-result-and-consumers.md`: retorno booleano após efeitos/notificação, callers de derrota e limites numéricos de Easing.

- `docs/61-initial-attribute-affinity.md`: pares/taxas iniciais comprovados por código e metadata; seletor e matriz portados/testados9/9.

- `docs/60-damage-pipeline-and-subtraction.md`: ordem de modificadores e Parameter.Sub portado; testes9/9, integração de combate pendente.

- `docs/59-combat-hit-threshold.md`: limiar de acerto portado e testado9/9; ordem do RNG e backend ainda a resolver.

- `docs/58-gameplay-combat-entry.md`: prioridade gameplay, entrada de combate, intervalos humano/inimigo e cadeia de dano/recompensas a recuperar.

- `docs/57-save-localization.md`: valor inicial de SAVEDATA_LOCALIZE, configuração por reflexão e filtros/normalização de ReadTranslate.

- `docs/56-save-strings.md`: framing de strings, escrita UTF-8, detecção na leitura e flag SAVEDATA_LOCALIZE; limites antes de codec DelayEvent.

- `docs/55-delay-event-save-block.md`: ligação de DelayEvent ao bloco2, layout e normalização de null/vazio; distinção dos métodos de scripts sem caller direto encontrado.

- `docs/54-script-payload.md`: formato BE das linhas de scripts, codec C++17 e testes8/8; distinção de DelayEvent e limites do leitor de buffer.

- `docs/53-checkscript-arrays-and-consumer.md`: construção recursiva de grupos, consumidor de NewGame, retornos constantes e delimitação da busca de campos; executor de scripts preparado para investigação.

- `docs/52-data-list-and-check-timing.md`: dez descritores tipados e seletores de save; GetTiming e preparação das tabelas de verificação. Quatorze novos FDEs,2.988 bytes, cobertura sintática integral; sem execução do jogo.

Os relatórios detalhados e o histórico ficam em `docs/research-log.md`, conforme AGENTS.md. Não publicar dados originais nem pseudocódigo; saídas nativas locais ficam em `build/`, ignorado pelo Git.

- `docs/04-android-startup.md`: boot Android, loop e input Java.
- `docs/05-il2cpp-metadata.md`: catálogo estrutural IL2CPP e candidatos a subsistemas.
- `docs/06-native-bootstrap.md`: `libmain.so` ARM64, tabela JNI e carregamento Unity/IL2CPP.

Endereços Ghidra da biblioteca ARM64 (base `0x00100000`): `JNI_OnLoad` em `0x00104a70`; NativeLoader.load em `0x00104b00`; unload em `0x00104bdc`; helper dlopen em `0x00104cd8`. São observações estáticas, não funções portadas ou verificadas em runtime.

- `docs/07-il2cpp-code-map.md`: tokens, relocations e endereços nativos.
- `docs/08-smali-validation.md`: ramos Java confirmados no bytecode.
- `docs/09-native-callbacks.md`: FDEs, chamadas e funções pequenas; falhas do pseudocódigo.
- `docs/10-unity-assets.md`: catálogo Unity, referências de cena e streams de áudio.
- `docs/11-resource-reader.md`: caminho AssetReader/Decode e pausa da extração conforme skill.
- `docs/12-asset-extraction.md`: retomada autorizada, inicialização da chave, contêineres e extração validada.
- `docs/13-animation-formats.md`: inventário INF/SEB e leitores de animação.
- `docs/14-unity-media.md`: áudio, fontes e texturas Unity, com limites das conversões.
- `docs/15-native-assets.md`: leitor C++17 e diferencial dos contêineres.
- `docs/16-sprite-layout.md`: campos geométricos e atlas otimizados.
- `docs/17-random-selection.md`: seleção das implementações RNG e wrappers.
- `docs/20-persistence-entrypoints.md`: pontos de entrada de persistência e lacunas.
- `docs/19-asset-viewer.md`: viewer SDL3 opcional e captura RGBA 1:1 validada.
- `docs/18-optimized-textures.md`: OPT/OPTINFO, remapeamento e referências visuais com limites explícitos.
- `docs/22-unity-lifecycle.md`: callbacks do framework Unity e mapa estático de chamadas.
- `docs/21-image-resource-mapping.md`: carregamento de Image, dimensões e opções.
- `docs/23-android-oracle.md`: ambiente conferido e próximo cenário de comparação com o original.
- `docs/24-frame-conditions.md`: filtros de Update/OnGUI, pausa, splash, espera e encerramento por assembly.
- `docs/25-resource-catalog-audit.md`: 51 catálogos, IDs, variantes e candidatos de arquivos sem presumir fallback.
- `docs/26-resource-aliases.md`: regras condicionais GIF/PNG, MLD/OGG e busca JarInflater por candidatos.
- `docs/27-frame-policy.md`: origem do alvo30, overrides120/400/2, precedência do getter e separação entre frames e eventos lógicos.
- `docs/28-form-events.md`: despacho virtual Init/Update/Draw/Resume/Suspend/Finish e cálculo de passagens lógicas.
- `docs/29-seb-animation-timing.md`: Frame manual, lista automática, resets, condições de desenho e contador separado por entidade.
- `docs/30-animation-transitions.md`: Change/CutIn/Restore, predicados ECS, módulo C++17 e testes de contrato.
- `docs/31-animation-producers.md`: criação/reutilização de componentes, valores explícitos e condições nas factories de entidade.
- `docs/32-ecs-animation-order.md`: registro por interfaces, cadeia QuestForm→AnimationSystem e iterações extras ECS.
- `docs/33-ecs-lifecycle.md`: diferença entre flags/hooks/notificações, pausa, limpeza e reconstrução por stream.
- `docs/34-animation-emitter-projectile.md`: condições dos callers restantes AddAnimation, partícula versus emissor e anim versus rotate.
- `docs/35-component-slots.md`: armazenamento concreto BaseEntity, callbacks e módulo C++17 validado, sem generalização às factories ainda virtuais.
- `docs/36-pause-hooks.md`: IPausable resolvido, hooks BasePauseSystem e ordem de World.Deserialize.
- `docs/37-entity-factory-and-subsets.md`: fábrica concreta BaseEntity/35 posições, inscrição de callbacks e reavaliação de grupos distinta da notificação Change.
- `docs/38-component-predicates.md`: ContainsAll/Any nativos, ordem de Filter.Matches, getters e sobrecarga silenciosa de Subset.Add.
- `docs/39-generic-collections.md`: referências HashSet<Entity>/IsNullOrEmpty<int>, duplicatas, wrappers de critérios e predicado portátil validado.
- `docs/40-entity-enumeration.md`: comparador por identidade/hash de ID, enumerador efetivo da animação, versão e reuso de posições.
- `docs/41-entity-load-and-destruction.md`: desligamento de callbacks após remoções, limpeza do registro e duas passagens de desserialização.
- `docs/42-component-save-fields.md`: payloads Animation/Direction/Seb, largura/sinal, layers compacto, booleano e loop de leitura incompleta.
- `docs/43-animation-payload-codec.md`: ordem big-endian demonstrada, codecs fixos Animation/Direction e testes de todos os padrões16 bits.
- `docs/44-sprite-payload-codec.md`: codec Seb/layers com null/vazio, valores assinados, limites distintos de escrita/leitura e consumo de prefixo.
- `docs/45-entity-write-layout.md`: hooks extras vazios dos três componentes e escrita em duas passagens com presença por slot.
- `docs/46-save-order-and-callers.md`: enumeração Dictionary.Values, PackSave/LoadGame e bloco13 entregue a ByteArrayInputStream.
- `docs/47-save-stream-eof.md`: cadeia MemoryStream→InputStream converte EOF0 em-1; limite da observação genérica de StreamUtil e preparação ainda pendente.
- `docs/48-system-save-and-data-loading.md`: InitSystemSaveData, alvo BaseData[] de DeserializeData e despacho normal/compartilhado anterior ao ECS.
- `docs/49-continue-game-preparation.md`: caller TitleForm, preparação NewGame/NewData por hooks virtuais e InitWorld após carga.
- `docs/50-world-destroy-and-title.md`: ordem de Destroy, ausência de reset direto do flag e limites da busca de callers/Init/Finish do título.


## 2026-10-25 ? BaseData.NewGame

Auditoria est?tica de22 tipos/10 implementa??es e4 callees (1.556 bytes, cobertura sint?tica integral). Find resolvido por vtables, distin??o QuestData documentada; arrays de AreaData reinicializados. Ver docs/51-basedata-newgame-overrides.md. Limpeza pr?via do World continua desconhecida; nenhum reset aproximado implementado. Sem execu??o do APK/saves ou mudan?a C++.

## 2026-10-25 — Fronteira Java de métricas

Relatório98: Init14724 liga Utility ao delegate15013; DEX confirma Paint/SANS_SERIF e payload de duas medidas. Leitor14446 e conversão13770/13777 rastreados; backend efetivo nas falas desconhecido. Pesquisa estática, sem código decompilado versionado ou nova dependência.

## 2026-10-25 — Seleção de fonte Unity

Relatório99: _getDefaultGUIFont17808 resolve caminhos por idioma e invalida cache; GetLanguageCode14019 usa langPackCode_ ou ja-JP/en-US. Cruzadas quatro chaves de ResourceManager. Fallback Unity e fonte efetiva ainda desconhecidos; nenhuma aproximação implementada.

## 2026-10-25 — Código dos pacotes de idioma

Relatório100: stores de Language.langPackCode_, sanitização de @language e normalização conferidos. Cruzamento com três CSVs e leitura de preferência em Init. Cinco métodos/37052 bytes de disassembly sintaticamente cobertos; compreensão restrita aos trechos documentados, nenhuma execução.

## 2026-10-25 — Construção da fonte das falas

Relatório101: Start18197 chama Language.Init; CreateTalkForm36177 cria TextLayout com fonte padrão. Construtores e campos conferidos, incluindo autoScale/localizeFontEnable e objetos fontBuf_/font_ distintos. Investigação estática, backend após ajustes ainda aberto.

## 2026-10-25 — text_fit

Portado recorte0x1bbd32c..0x1bbd420 de SetText19198: ajuste de blocos com métricas via host. tools/build-desktop.cmd, Windows/MSVC14.51:19/19 CTests (build/research/text-fit-build.log). Sem MATCH contra original: Android indisponível, métricas reais/parser não integrados. Relatório102.

## 2026-10-25 — Preparação de tamanho de texto

text_fit ampliado com correção inicial recuperada de SetText19198. tools/build-desktop.cmd, Windows/MSVC14.51:19/19 CTests, build/research/text-font-preparation-build.log. Sem MATCH de runtime; requestFontScale explícito. Relatório103.

## 2026-10-25 — Localização da fonte

Relatório104 fecha guardas/pilha/fatores de localização e restauração no helper0xf875bc. Integração ainda depende de AdjustFontSize e cobertura de exceções anterior ao contexto. Sem novo código ou teste de runtime.

## 2026-10-25 — Ajuste e preparação da fonte

Relatório105: sequência de AdjustFontSize e nove chamadas da preparação cruzadas com LSDA0x935e34. Caminhos anteriores ao contexto pulam o helper interno; chamadas posteriores passam por ele. Sem implementação aproximada ou novo teste de runtime.

## 2026-10-25 — text_font

Portados recortes síncronos de _setSize17770, UpdateFontScale17806 e aritmética posterior à atualização em _adjustFontSize17796. Windows/MSVC14.51, tools/build-desktop.cmd:20/20 CTests, log build/research/text-font-state-build.log. Sem MATCH com APK ou validação Linux; dispatcher e parser não integrados. Relatório106.

## 2026-10-25 — Presets e origem de configuração da fonte

Relatório107 resolve arrays iniciais por FieldRef/default_data e hashes. Graphics.Init fornece writer concreto de useFontTexture_; Language contém writes inline de escala. Novos exports Canvas.Init/Graphics.Init com cobertura sintática integral, sem execução Android. Último build20/20 preservado; não significa MATCH de runtime.

## 2026-10-25 — Fonte e invalidação de layout em Draw

Relatório108 distingue cópia via GetFont/SetFont, configuração de Graphics e escala de TextLayout.Draw. Sete exports/6256 bytes sintaticamente cobertos; nenhum MATCH de runtime alegado. Último build20/20 preservado, sem alterações C++.

## 2026-10-25 — Cenário de tabela no nativo

event_trace compõe módulos existentes com scripts locais e fixtures explícitas. tools/build-desktop.cmd:20/20; execução EN/JA registro10 com checks49/50 e callbacks passou. Logs em build/research/event-trace-build.log e event-scenario-{en,ja}.json. Sem MATCH Android, termos não avaliados e UI não executada. Relatório109.
