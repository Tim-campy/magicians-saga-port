# Índice de relatórios por capítulos

Leia primeiro o [plano e mapa do código](plan.md), o [progresso atual](progress.md) e o [registro de mudanças](../CHANGELOG.md). O [diário de pesquisa](research-log.md) conserva a sequência dos trabalhos; [DECOMPLOG](../DECOMPLOG.md) acompanha a reconstrução.

Os números dos relatórios são identificadores cronológicos estáveis. A divisão abaixo é temática e segue os marcos M0–M6; não indica que um capítulo esteja concluído. Relatórios antigos descrevem a evidência e os limites da ocasião.

Ferramentas e instalação: [tooling.md](tooling.md). Fluxo de versionamento: [CONTRIBUTING.md](../CONTRIBUTING.md).

## 1. Reconhecimento e arquitetura — M0

Inventário e base técnica; marco ainda parcial.

- [01 — Inventário do APK — M0](01-apk-inventory.md)
- [02 — Arquitetura — M0 parcial](02-architecture.md)
- [03 — Perguntas em aberto](03-open-questions.md)
- [04 — Inicialização Android e fronteira nativa](04-android-startup.md)
- [05 — Catálogo IL2CPP — metadados versão 31](05-il2cpp-metadata.md)
- [06 — Bootstrap nativo ARM64](06-native-bootstrap.md)
- [07 — Metadados IL2CPP para endereços ARM64](07-il2cpp-code-map.md)
- [08 — Conferência do bootstrap e loop em smali](08-smali-validation.md)
- [09 — Callbacks nativos: chamadas e limites](09-native-callbacks.md)

## 2. Assets e apresentação — M1

Extração, leitores e visualização técnica; cena, fontes e áudio integrados permanecem pendentes.

- [10 — M0 — Catálogo estático dos recursos Unity](10-unity-assets.md)
- [11 — Leitura de recursos: caminho estático e decisão pendente](11-resource-reader.md)
- [12 — Extração comprovada dos contêineres de recursos](12-asset-extraction.md)
- [13 — M0 — Inventário de animações `.seb` e índices `.inf`](13-animation-formats.md)
- [14 — M1 — áudio, fontes e texturas Unity](14-unity-media.md)
- [15 — Primeiro leitor nativo de recursos — contêiner JarInflater](15-native-assets.md)
- [16 — M0 — Layout geométrico dos sprites SEB](16-sprite-layout.md)
- [18 — M0 — Mapeamento OPT de texturas otimizadas](18-optimized-textures.md)
- [19 — Visualizador técnico nativo de PNG e OPT](19-asset-viewer.md)
- [21 — Image: dimensões e resolução de recursos](21-image-resource-mapping.md)
- [25 — Auditoria estática dos catálogos de recursos](25-resource-catalog-audit.md)
- [26 — Conversão de nomes e busca de recursos](26-resource-aliases.md)

## 3. Boot, menus e entrada — M2

Ciclo de atualização e despacho de formulários; navegação integrada ainda não implementada.

- [22 — Ciclo Unity: entradas do framework](22-unity-lifecycle.md)
- [24 — Condições estáticas de atualização, desenho e pausa](24-frame-conditions.md)
- [27 — Política estática de FPS e campos de taxa](27-frame-policy.md)
- [28 — Eventos lógicos de formulário e contagem de passagens](28-form-events.md)

## 4. Gameplay e simulação — M3

Prioridade atual. Regras de combate, animação, ECS, recompensas e eventos; módulos isolados ainda não formam um loop jogável.

- [17 — RNG: cadeia de chamadas e seleção pendente](17-random-selection.md)
- [29 — Avanço estático de animações SEB](29-seb-animation-timing.md)
- [30 — Transições de animação por entidade e primeiro módulo de regras](30-animation-transitions.md)
- [31 — Produtores dos componentes de animação](31-animation-producers.md)
- [32 — Registro e despacho do sistema de animação ECS](32-ecs-animation-order.md)
- [33 — Habilitação, pausa e reconstrução dos sistemas ECS](33-ecs-lifecycle.md)
- [34 — Animação criada por emissores e projéteis](34-animation-emitter-projectile.md)
- [35 — Armazenamento e notificações de componentes](35-component-slots.md)
- [36 — Interface de pausa e preparação do carregamento do mundo](36-pause-hooks.md)
- [37 — Fábrica concreta e manutenção dos grupos ECS](37-entity-factory-and-subsets.md)
- [38 — Predicados de componentes e caminho de leitura dos grupos](38-component-predicates.md)
- [39 — Coleção de entidades e resolução dos filtros genéricos](39-generic-collections.md)
- [40 — Comparador e ordem de visita das entidades](40-entity-enumeration.md)
- [58 — Gameplay prioritário: entrada no combate e intervalos de ataque](58-gameplay-combat-entry.md)
- [59 — Acerto no combate: limiar e consumo de RNG](59-combat-hit-threshold.md)
- [60 — Modificadores de dano e subtração do parâmetro de destino](60-damage-pipeline-and-subtraction.md)
- [61 — Tabelas iniciais de afinidade e implementação nativa](61-initial-attribute-affinity.md)
- [62 — Resultado do dano, notificações e consumidores](62-damage-result-and-consumers.md)
- [63 — Evento de dano e recorde de quest](63-damage-event-and-quest-record.md)
- [64 — Fatores numéricos de dano](64-combat-easing-factors.md)
- [65 — Derrota e ordem das recompensas](65-defeat-and-reward-order.md)
- [66 — Coleta e acumulador de recompensas da fase](66-drop-collection-and-stage-rewards.md)
- [67 — Resultado da quest e entrega de recompensas](67-quest-result-reward-grant.md)
- [68 — Estados de saída e guardas do resultado](68-stage-exits-and-result-guards.md)
- [69 — Finalização dos membros e payload de retorno](69-member-finish-and-return-payload.md)
- [70 — Tesouros: descoberta, estoque e recrutamento](70-treasure-inventory-grants.md)
- [71 — Handler de retorno da quest](71-town-return-handler.md)
- [72 — Evento de retorno: executor e lacuna nos dados](72-return-event-dispatch-and-data-gap.md)
- [73 — Indexação dos dados pelo ID](73-game-data-id-indexing.md)
- [74 — Origem textual das tabelas de gameplay](74-game-data-text-source.md)
- [75 — Encoding e terminações das tabelas](75-table-encoding-and-line-endings.md)
- [76 — Tokenização e leitura dos eventos extraídos](76-event-table-tokenization.md)
- [77 — Instruções usadas nos eventos e continuação adiada](77-event-opcodes-and-delay.md)
- [78 — Retomada dos eventos adiados](78-delay-event-resumption.md)
- [79 — Guardas da atualização de eventos de gameplay](79-gameplay-event-update-guards.md)

- [81 — Origem das continuações e identidade dos scripts](81-delay-origin-and-script-identity.md)
- [82 — Executor nativo de scripts e continuações por contagem](82-native-event-flow.md)
- [83 — Efeitos de progressão: dinheiro, cristais, áreas e missões](83-event-progression-effects.md)
- [84 — Stamina e encaminhamento dos eventos de apresentação](84-stamina-and-event-dispatch.md)
- [85 — Preparação dos textos de fala e de evento](85-presentation-text-preparation.md)
- [86 — Produtor do catálogo de falas e fronteira de tradução](86-talk-catalog-producer.md)
- [87 — Divisão nativa dos textos e regra exata de trim](87-native-presentation-split.md)
- [88 — Substituição nativa dos argumentos de apresentação](88-native-presentation-arguments.md)

## 5. Persistência e progressão — M4

Preparação de dados, serialização e codecs; save completo e integração ainda pendentes. Recompensas e eventos também estão no capítulo4.

- [20 — Persistência: pontos de entrada, sem formato recuperado](20-persistence-entrypoints.md)
- [41 — Destruição e carregamento de entidades em duas passagens](41-entity-load-and-destruction.md)
- [42 — Campos persistidos de animação, direção e SEB](42-component-save-fields.md)
- [43 — Ordem dos bytes e codecs de payload fixo](43-animation-payload-codec.md)
- [44 — Codec de payload SEB e camadas](44-sprite-payload-codec.md)
- [45 — Hooks de componentes e layout da escrita de entidades](45-entity-write-layout.md)
- [46 — Ordem do registro e localização do bloco do mundo](46-save-order-and-callers.md)
- [47 — Stream concreto do save e comportamento em EOF](47-save-stream-eof.md)
- [48 — Inicialização do save de sistema e carga de BaseData](48-system-save-and-data-loading.md)
- [49 — Preparação no caminho Continuar](49-continue-game-preparation.md)
- [50 — Destruição do mundo e limites do caminho do título](50-world-destroy-and-title.md)
- [51 — Inicialização dos dados antes de Continuar](51-basedata-newgame-overrides.md)
- [52 — Descritores de dados e preparação das verificações](52-data-list-and-check-timing.md)
- [53 — Forma dos grupos e consumidor em NewGame](53-checkscript-arrays-and-consumer.md)
- [54 — Bloco de scripts: formato e codec nativo](54-script-payload.md)
- [55 — Eventos adiados no bloco2 do save](55-delay-event-save-block.md)
- [56 — Strings do save: comprimento, encoding e localização](56-save-strings.md)
- [57 — Configuração e filtros de tradução do save](57-save-localization.md)

## 6. Validação desktop e organização — M5

Builds Windows e contratos locais; comparação dinâmica, Linux e release candidata pendentes.

- [23 — Próxima etapa: comparação com o Android original](23-android-oracle.md)
- [80 — Reestruturação do projeto e início do versionamento](80-project-restructure.md)

## 7. Viabilidade Nintendo 3DS — M6

Sem relatório de benchmark. Diretório reservado; nenhuma viabilidade declarada.
