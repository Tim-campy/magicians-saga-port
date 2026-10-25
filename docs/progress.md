# Progresso

Atualização: 2026-10-25. **M0 parcial; leitor nativo de assets implementado, sem port de gameplay.**

## Concluído

- APK identificado, hash preservado, inventários ZIP/manifesto/ELF/assets registrados.
- Unity/IL2CPP evidenciado; estrutura inicial e três relatórios obrigatórios criados.
- REA/JADX operacionais pelo wrapper local; bootstrap Java e smali conferidos (`04`, `08`).
- Ghidra direto ARM64 operacional; bootstrap JNI e carregamento Unity/IL2CPP mapeados (`06`).
- Metadata v31: 64 images, 8.619 tipos e 63.273 métodos (`05`).
- Mapa de 56.955 entradas nativas não nulas e três callbacks main.Main (`07`).
- Intervalos FDE e chamadas dos callbacks inventariados; viewport, seleção de FPS e caminho de volume examinados (`09`).
- Apktool 2.12.1 e ADB 1.0.41 instalados localmente e verificados, sem alterar PATH global.
- Catálogo Unity: 2.389 objetos, cena ligada ao script main.Main e 23 streams de áudio presentes no APK; duas execuções idênticas e cinco entradas inválidas rejeitadas (`10`).
- Leitor C++17 JarInflater compilado em Windows x64; CTest e diferencial dos 30 contêineres/1.963 membros passaram (`15`). Caminhos Unicode Windows verificados.
- Leitor OPT nativo passou no diferencial de 286 arquivos, 2.559 células e 2.481 peças; build/CTest 2/2 e regressão Jar passaram.
- Leitor SEB nativo passou no diferencial de 303 arquivos, 452 grupos e 2.686 registros; build/CTest 3/3 e regressões Jar/OPT passaram.

## Ativo

- **Prioridade: gameplay**, conforme pedido do usuário. Foco em combate, coleta/recompensas e eventos de progressão, buscando uma primeira simulação determinística integrada. Módulos isolados já existem; ainda não há loop jogável.
- Guardas da fila aprofundadas (`79`): retorno antecipado por +0xc0 e por TapjoyForm.AcquireBalance impede avançar esperas nessa passagem; +0xd0>2 pula introdução/guias, mas alcança a fila. AppData.Push apenas encaminha ao FormManager; produtor de SubForm+0x1d0 e eventos por hora seguem pendentes. Nenhum teste dinâmico necessário nesta etapa.

- Retomada de DelayEvent localizada (`78`): SelectForm decrementa wait só com hour=-1, remove antes de retomar e usa limite inicial de iteração. SubForm.Finish retoma continuação por outra rota; restaura talkChara em slot5 versus7. Guardas, espera por hora e atribuição ao formulário pendentes.

- Dispatch dos scripts mapeado (`77`): oito opcodes nas208 linhas;10 agenda DelayEvent e interrompe executor da matriz, não dispatcher unitário. Identificados alvos de moeda/cristais/áreas/falas/eventos e substituição de argumentos sentinela. Scheduler/efeitos completos ainda pendentes.

- Eventos extraídos interpretados (`76`): tokenização TAB preserva vazios; matrizes com contagens e null externo recuperadas. Ferramenta leu228 registros/208 linhas de execução por idioma sem sobras, timing0 em todos. Verificados limites/truncamentos; delegate e origem final do contêiner ainda pendentes.

- Encoding/CRLF esclarecidos (`75`): JString.New usa UTF-8; normalização CRLF condicionada a Unity>=6000. Auditoria reproduzível de44 textos: UTF-8 válido, sem BOM,5103 CRLF. No caminho sem normalização CR segue ao parser; tokenizador/delegate ainda pendentes.

- Origem textual dos dados ligada (`74`): Data.Load constrói JarInflater, lê21 tabelas incluindo Event.txt e passa sequências ao loader. Split usa LF, remove segmentos vazios, sem trim/escape nessa chamada; normalização condicional, encoding e delegate ainda pendentes antes do loader integrado.

- Indexação de Data.Load recuperada (`73`): array por maior ID+1, lacunas null, última duplicata vence. Aplicada às tabelas extraídas de eventos:229 posições, lacuna180,1133 fora. Origem da sequência/delegate ainda pendentes antes de concluir o array efetivo e implementar loader.

- Executor do evento1133 auditado (`72`): índice fora de Data.eventData retorna null; evento válido incrementa execNum antes de verificar script. Event.txt inglês/japonês têm228 linhas e nenhum ID1133, mas dimensionamento/população do array ainda pendentes. Não atribuída penalidade nem no-op definitivo; próxima frente é Data.Load.

- Assinante do retorno confirmado (`71`): TownSystem.Init inscreve OnReturnFromQuest no evento31/QuestResult. Handler executa evento1133 para código1, eleva high score de dano e marca retorno na seleção. Evento1133 e consumidor do flag ainda pendentes; não há concessão direta adicional no corpo auditado.

- Inventário de tesouros aprofundado (`70`): slot8 é BaseData.Find nos quatro tipos; equipamentos/roupas/chapéus somam estoque com teto1 após wrap32, recrutamento marca recruit=1 e Find(true). Regra de estoque portada e testada, Windows/CTest10/10. Integração por IDs e assinantes do retorno pendentes.

- OnFinish e evento de retorno resolvidos (`69`): copia nível/EXP de members_ para CharaData, destrói subMembers_ e limpa listas; não é restauração de HP nem save em disco comprovado. Genérico do resultado confirmado como MemberSystem; evento31 leva QuestResult(battleResult,areaId,stage). Assinantes/inventário ainda pendentes.

- Guardas de resultado/saída confirmadas (`68`): chefe derrotado + recompensa coletada levam ao estado3; pulso de entrada chama resultado, depois estado4 retorna código0. Morte principal/tempo usam códigos1/2; saída por diálogo usa3. Caller extra de resultado exige Config.DEBUG. Callback emite evento31, ainda pendente de resolução.

- Entrega de recompensas aprofundada (`67`): ShowQuestResult soma bônus de tesouros, chama AddMoney/AddCrystal, registra resgatados e distribui EXP integral a entidades elegíveis por parâmetro0>=1. Custos de nível usam tabela e multiplicador; saldo permanente soma64 bits. Callers/estados, itens e wrappers ainda pendentes antes da integração.

- Coleta e acumulador da fase recuperados (`66`): rota animada com contador31 e rota direta para recompensa de chefe. Portada contabilização de EXP/dinheiro/prisioneiros/tesouros com wrap32 e duplicatas; build Windows/CTest10/10 passou. Entrega final ao jogador e integração de coleta/animação ainda pendentes.

- Derrota/recompensas mapeadas (`65`): dois contadores distintos, EXP por nível, primeiro sucesso na lista de tesouros e consumo de Hits1000 preservados. CreateDrop tipo3 substitui escala por100; coleta ainda desconhecida. Seis helpers exportados integralmente, offsets confirmados; sem C++ especulativo. Próximo: coleta, flags e RNG.

- Fatores numéricos de combate portados (`64`): dois casos de Easing com amplitude zero, operações float32 separadas e wrap32 do bônus. Modelo estático conferiu 148 pontos internos; build Windows/CTest 9/9 passou (`combat-easing-build.log`). RNG, colisões, derrota/recompensas e integração continuam pendentes. Não houve validação dinâmica.

- Evento15 ligado a StageSystem.OnDamage (`63`): soma dano informado contra receptores inimigos ao total, sem limitar ao HP perdido. Start zera total; UpdateQuestRecord pode elevar bestTotalGaveDamage e GetBestTotalGaveDamage consulta máximo entre quests. Outros assinantes/callers ainda pendentes; nenhuma integração aproximada.

- Retorno e consumidores de Damage esclarecidos (`62`): após efeitos e World.Send15, testa value+extra<1; não retorna quantidade de dano. OnBulletHit e um ramo de OnCollide chamam Defeat, outro muda estado humano. Easing usa float32 e seno ainda executado; integração deve preservar ordem de callbacks e precisão.

- Tabelas iniciais de afinidade recuperadas e portadas (`61`): cinco pares e taxas[90,45,20,60], com blob/hash da metadata confirmado. Matriz25 combinações e casos limites verificados; build Windows/CTest9/9 passou. Dados tratados como estado explícito, sem presumir imutabilidade. Próximo: Easing/efeitos e integração do dano.

- Pipeline de dano aprofundado (`60`): defesa/atributos, variação, ramo de170%, fatores Easing e mínimo aleatório separados em ordem. Parameter.Sub portado com máximo0 sem clamp, extras excluídos, retorno de excesso e wrap32; build Windows/CTest9/9 passou. Tabelas, Easing e efeitos pós-subtração pendentes antes de integrar combate.

- Regra de acerto portada (`59`): base90/80, ajuste clamp[-60,15], metade truncada e wrap32 preservados. Limiar humano pode105; RNG continua sendo consumido. C++17/build Windows/CTest9/9 passou. Gerador concreto, cálculo de dano e integração de HP ainda pendentes.

- **Prioridade do usuário: gameplay.** Saves/localização em segundo plano. Combate iniciado em`58`: intervalos distintos de humano/inimigo recuperados; BattleSystem.Update não processa ataques em seu laço local. Damage/Defeat e dependências de parâmetros, RNG, afinidade e drops exportados. Próximo: chance/acerto/dano e consumo dos intervalos, depois morte/recompensas.

- Localização de save aprofundada (`57`): SAVEDATA_LOCALIZE inicia false, mas Config.Create usa FieldInfo.SetValue; valor permanente ainda não comprovado. ReadTranslate tem guarda própria, filtros e normalização de chave/retorno identificados. Origem da tabela, configuração efetiva e WriteTranslate seguem pendentes.

- Strings do save aprofundadas (`56`): escrita UTF-8 com comprimento int32 BE em bytes; leitura detecta encoding, inicialmente UTF-8/Shift_JIS, com tratamento de BOM e flag Config.SAVEDATA_LOCALIZE. Framing confirmado; política completa de leitura/tradução ainda pendente antes de codec DelayEvent.

- Bloco2 do save ligado a DelayEvent (`55`): PackSave grava coleção+0x200; LoadGame substitui coleção e lê eventos. Cinco campos inteiros, string e array identificados, com null/vazio normalizados. Formato de string e consumidor temporal pendentes. Nenhum caller direto de SerializeScripts/DeserializeScripts encontrado; codec54 não foi ligado indevidamente ao save.

- Codec de bloco de scripts implementado (`54`): coleção de linhas int32 big-endian, linhas vazias, quantidade externa negativa→vazio e consumo de prefixo. Fixture, truncamentos e contagens inválidas verificados; build Windows/CTest8/8 passou. Não é save completo nem serializer de DelayEvent; callers e semântica de opcodes seguem pendentes.

- Grupos de verificação esclarecidos (`53`): MakeArray<BaseData[][]>([5,0]) cria cinco linhas vazias distintas no caminho bem-sucedido; NewGame percorre grupos com CheckNotFind/CheckTerm, ambos retornos zero nos tipos auditados. Executor e persistência de scripts exportados como próxima frente; grupos vazios não implicam scripts ausentes.

- GetDataList resolvido (`52`): dez arrays tipados com ordem e seletores de save concretos. InitCheckScriptData agrupa por slot14/GetTiming; tipos examinados retornam zero, enquanto o laço compara índices positivos. ArrayInit preenche outro array com10000. MakeArray, população de Data e consumidores ainda pendentes; limpeza ECS não presumida.

- Overrides de BaseData.NewGame auditados (`51`):22 tipos/10 corpos; slot8 resolvido como Find, com override condicional de QuestData. AreaData.oldNewGame recria arrays e sobrescreve estado local. Ainda não comprova limpeza ECS. Nenhum save acessado ou C++ alterado.

- World.Destroy mapeado (`50`): operações em coleção→entidades→sistemas→eventos; sem reset direto de byte+0x38. Busca BL/B não achou caller direto. Init/Finish do título não chamam limpeza ECS diretamente; preparação pré-carga permanece aberta a efeitos transitivos/indiretos, sem reset arbitrário no port.

- Caller Continuar recuperado (`49`): TitleForm chama NewGame→LoadGame(slot0)→InitWorld→SelectForm. NewGame passa por NewData, que despacha BaseData.NewGame por objeto antes de InitCheckScriptData. Limpeza ECS ainda não comprovada; investigar descritores/tipos concretos e transições anteriores, sem executar rotinas com efeitos em saves.

- InitSystemSaveData e DeserializeData esclarecidos (`48`): o primeiro cria SecureProperty+0x190; o segundo carrega registros para BaseData[] com hooks virtuais normal/compartilhado. Ordem dados→ECS confirmada, mas limpeza prévia do World ainda depende de descritores/overrides/callers. Nenhum reset foi presumido.

- EOF do bloco13 fechado (`47`): ByteArrayInputStream usa MemoryStream e herda InputStream.Read, que converte zero em-1. Leitura incompleta chega ao erro de StreamUtil, não ao loop genérico sem progresso. LoadSystem/DeserializeData exportados; seus efeitos completos e preparação do mundo permanecem pendentes.

- Ordem Dictionary.Values e callers localizados (`46`): enumeração crescente por entradas ativas com versão; PackSave seletor13 grava o mundo, LoadGame obtém elemento13 de SecureProperty e fornece ByteArrayInputStream a World.Deserialize. LoadSystem/DeserializeData e EOF do stream concreto são próximos alvos; preparação do registro ainda não comprovada.

- Hooks extras Animation/Direction/Seb confirmados como RET (`45`); layout de escrita EntityManager recuperado: contador/quantidade, todos os cabeçalhos, depois todos os blocos de componentes. Bloco começa com quantidade de slots e inclui presença por índice. Ordem Dictionary e demais componentes ainda pendentes antes de save completo.

- Codec de payload Seb/layers implementado (`44`) com null/vazio distintos, valores assinados, consumo de prefixo e assimetria leitura127/escrita<127. Build Windows/CTest7/7 passou com teste de payload ampliado. Ainda é codec de buffer completo, sem emulação do stream original ou envelope do save.

- Ordem big-endian de short/int comprovada por BitConverter+inversão (`43`). Codecs fixos AnimationPlayback/DirectionState implementados com sinal/truncamento explícitos, fixtures e cobertura exaustiva16 bits; build Windows/CTest7/7 passou. Ainda sem codec Seb variável nem leitura completa de saves/stream incremental.

- Payloads Animation/Direction/Seb mapeados (`42`): campos byte/short com sinal, booleano igual a1, layers null/vazio/elementos distintos e comprimento<127. Leitor acumula blocos e não termina se retornar zero repetidamente antes de completar. Endianness/conversores e stream concreto pendentes antes de codecs nativos.

- Carregamento e destruição aprofundados (`41`): componentes são removidos antes de DisposeEvents; DestroyAll limpa o dicionário após percorrer entidades. Deserialize registra todos os IDs/callbacks antes da segunda passagem de componentes. Preparação do caller, conteúdo dos componentes e contrato de stream ainda pendentes; nenhum leitor de save aproximado implementado.

- Comparador e enumerador da animação fechados (`40`): igualdade por referência, hash pelo campo de ID mascarado, visita crescente por slots ativos e checagem de versão. Remoção libera posição reutilizada por inserções futuras; crescimento mantém posições. Capacidades, escritores de ID e limpeza ainda pendentes antes de portar o conjunto.

- Referências genéricas resolvidas (`39`): coleção é HashSet<Entity>, helper é IsNullOrEmpty<int>. Predicado AND/OR/NOR portado sem presumir hash/identidade do filtro; CTest6/6 passou. Inserção duplicada não altera HashSet, mas Subset.Add ainda pode notificar. Comparador e ordem de enumeração permanecem pendentes.

- ContainsAll/ContainsAny portados com avaliação sequencial e curto-circuito (`38`), incluindo comportamento de lista vazia e índices inválidos alcançados. CTest5/5 passou. Filter.Matches e getters de Subset examinados: get_entities retorna a coleção+0x30 diretamente; helper genérico de vazio e semântica das coleções ainda precisam ser resolvidos.

- Fábrica concreta resolvida (`37`): World cria MyEntityManager com35 posições; override Create encaminha à base e tipo0 aloca BaseEntity. Inscrições de callbacks e diferenças entre reavaliação de grupos por Add/Remove e notificação por Change recuperadas. Coleções genéricas/filtros/cache ainda pendentes antes da integração.

- Contrato concreto BaseEntity portado para ComponentSlots C++17 (`35`): duplicatas, trocas, null, notificações após mutação e varredura com callbacks preservados. Revisão independente e CTest5/5 passaram em Windows. Ainda sem integração com grupos/pools/cena.
- Interface IPausable e hooks BasePauseSystem resolvidos (`36`); World.Deserialize carrega entidades antes de sistemas e só então grava byte+0x38. Preparação anterior ao carregamento ainda desconhecida.

- M0: aprofundar formatos, desenho, persistência, áudio e dependências de serviços.
- Pesquisa estática retomada por solicitação do usuário: catálogos auditados (`25`), conversão GIF/PNG e busca JarInflater mapeadas (`26`), filtros de callbacks e política de FPS aprofundados (`24`, `27`). Ainda há caminhos úteis sem executar o APK.
- Evento lógico1 ligado por vtables/switch a FormBase.Update; eventos0–5 mapeados e fórmula de passagens recuperada (`28`). Avanço SEB manual/automático, deduplicação e contador de animação por entidade documentados (`29`), sem simulação aproximada.
- Change/CutIn/RestoreAnimation recuperados por assembly e portados para módulo C++17 independente (`30`). Predicados diferentes de humano/monstro/inimigo, ordem de mutação e wrap32 preservados; build Windows e CTest4/4 passaram. Não integrado ainda a ECS, scheduler ou cena.
- Produtores de Animation/Direction/Seb e cinco factories examinados (`31`): valores explícitos, presença condicional e frame inicial de inimigos dependente dos dados. Cadeia QuestForm→World→SystemManager→AnimationSystem confirmada, incluindo filtros por interface e passagens extras (`32`).
- Ciclo ECS aprofundado (`33`): enable/disable chamam hooks/notificações sem remoção direta da lista; pausa por propriedade difere de PauseSystems; remoção total notifica antes de limpar. Sete callers diretos de AddAnimation conferidos, incluindo Emitter/Projectile (`34`), todos1/-1 sob suas respectivas guardas.

## Retomada autorizada

- Usuário autorizou investigar a chave e extrair recursos. Extração comprovada de 30 contêineres, 1.963 membros e 973 PNGs; script e validações em `12-asset-extraction.md`.
- Opção `--unity-media`: 68 texturas convertidas com dados originais preservados, 23 pares FSB/WAV, cinco fontes e cinco texturas vazias registradas. `docs/14-unity-media.md` documenta validações e anomalias do original.
- SEB: parser estrutural e round trip dos 303 arquivos passaram; campos geométricos mapeados (`13`, `16`). OPT/OPTINFO identificados como etapa necessária para interpretar atlas; investigação em curso.
- OPT: 286 reconstruções estruturais e 286 visualizações de limites ocupados; 2.481 peças conferidas pixel a pixel. Os 18 casos fora da grade são registrados, sem inventar tamanho original (`18`).
- M1 parcial: visualizador técnico SDL3/stb opcional compilado; 286 composições OPT coincidem pixel a pixel com a referência Python, inclusive os 18 casos fora da grade. Não há renderer de cena do jogo nem gameplay (`19`).

## Limitações

- ADB funciona, mas não há dispositivo conectado nem emulador instalado nos caminhos conhecidos. Nenhum jogo foi executado.
- Pseudocódigo Ghidra de OnUpdate mistura callees; relatório usa chamadas do assembly/bytes com limites e divergências explícitos.
- Bridge REA Windows não admite ARM ELF no perfil anunciado; Ghidra headless direto funciona.
- Offline, assinatura, completude do conteúdo e fidelidade comportamental ainda não verificados.

## Próximo

1. Continuar estaticamente a cadeia ResourceManager/INF: origem dos overrides, possível ligação room/shop, IDs implícitos e precedência de idioma (`25`, `26`). Conversão GIF/PNG comprovada, com seletor inicial compatível.
2. Seguir inicialização de LoadSystem e efeitos de DeserializeData antes de World.Deserialize (`47`); recuperar SecureProperty/RecordStore e integridade externa. Filtros, comparador, enumeração e payloads de três componentes já estão documentados (`39`–`46`); faltam capacidades/limpeza/pools e outros componentes para integrar ECS/save. Manter investigação de Enemy, flags, AutoAnimation e relógios antes de um loop jogável.
3. Manter a comparação dinâmica pendente (`23`); o usuário não pode executá-la agora. Solicitar somente quando não houver avanço estático útil.

M1–M6 não concluídos; não há build jogável. Critérios de M0 em `03-open-questions.md`.

Direção atual: usuário pediu continuidade estática em 2026-10-25. A ausência de Android não bloqueia esta investigação. Auditoria de 51 catálogos/1.298 registros adicionada, três testes passaram; diferenças e candidatos explicitados em `25`. A validação comportamental permanece pendente, sem declarar o port completo.
