# Registro de pesquisa

## 2026-10-25 — Efeitos de progressão integrados ao executor

Ciclo anterior foi progresso com event_flow/CTest11/11. Conferidas vtables AreaData/QuestData e getter state==1; exports de Add/SubMoney/Crystal revelam piso somente na subtração e condição adicional SYSSAVEI_YUGAMI para cristais. Implementados efeitos1/2/4/15 e substituição de sentinelas, sem assumir valor inicial do parâmetro. Teste integra crédito→espera→missão→débito com cópia de argumentos; Windows/CTest12/12 passou. Evidência, aritmética e limites em83. Próximo: efeitos8/12/31, registros e origem do parâmetro de sistema. Nenhum save real ou APK executado.

## 2026-10-25 — Executor e fila de eventos nativos

O ciclo anterior foi progresso ao fechar identidade e delimitar esperas efetivamente presentes. Implementado event_flow para executor de matriz e passagem de contadores, preservando ordem de remoção/restauração/resolução, limite inicial, identidade e cópia de argi. Host explícito para efeitos não implementados; sem política arbitrária de frames ou associação ao SubForm. Testes de sequência, falhas, wrap32 e mutações de callbacks passaram; Windows/CTest11/11. Evidência, precondições e limites em82. Próximo: integrar identificação/resolução e efeitos unitários antes de ligar à progressão.

## 2026-10-25 — Identidade do script e origem de DelayEvent

Confirmado delayEvent_ em SubForm+0x1d0 por metadata/offsets. Varreduras limitadas de escrita/endereço e construtores não localizaram seu produtor; ausência não declarada. Caller direto do construtor DelayEvent localizado em ExecuteScript. GetDelayId percorre eventData, pula null e compara identidade da matriz; falha retorna-1 após caminho adicional. Auditoria:37 instruções de espera, valores1/3/5/30/50, nenhuma hora explícita. Evidência estática e limites em81. Próximo: executor e continuação por contagem preservando identidade, sem aguardar esclarecimento de rotas não demonstradas nos dados extraídos. Sem alteração nativa ou repetição de testes.

## 2026-10-25 — Reestruturação e Git

A pedido do usuário, registrado baseline Git787a81e e reorganizados módulos C++/CMake sem alterar algoritmos. Criado índice de80 relatórios por sete capítulos M0–M6, plano, changelog e fluxo de contribuição; progresso condensado. Builds padrão e viewer Windows passaram com10/10 testes cada. Artefatos proprietários continuam locais e ignorados; nenhum remoto ou publicação. Relatório80 descreve mudanças e limites. Próximo trabalho funcional mantém foco em gameplay e continuações de eventos.

## 2026-10-25 — Prioridade de gameplay e guardas dos eventos

Usuário priorizou gameplay. Investigadas guardas anteriores à fila de DelayEvent por assembly e mapa IL2CPP: +0xc0 e retorno verdadeiro de TapjoyForm.AcquireBalance permitem retorno antecipado; +0xd0>2 não pula a fila. AppData.Push encaminha ao FormManager, sem atribuição direta à continuação do SubForm. Exportados AcquireBalance, SetEvent e MakeEvent; evidência e limites em79. Confiança confirmada para instruções e alvos mapeados, sem validação de execução. Nenhum C++ alterado; último CTest10/10 preservado. Próximo: escritor de +0x1d0 e espera por hora, visando integração fiel dos eventos com combate/recompensas.

## 2026-10-25 — Consumidores de DelayEvent

Ciclo anterior foi progresso com dispatch dos opcodes. Busca direta de GetDelayScript encontrou SelectForm.Update/SubForm.Finish. Exportados consumidores, resolver e construtor; ordem de remoção antes da continuação, contagem inicial da lista, hour=-1 e cópia de argi confirmados. Slots5/7 de talkChara diferem entre rotas. Evidência em78. Sem C++ alterado/testes repetidos. Próximo: guardas anteriores, hour e atribuição da continuação.

## 2026-10-25 — Dispatch dos eventos e delay

Ciclo anterior foi progresso com parser dos228 eventos. Exportadas três sobrecargas ExecuteScript; contados oito opcodes nas208 linhas.10 tem tratamento especial no executor da matriz, cria DelayEvent e preserva continuação atual+1. Dispatcher copia linha e resolve sentinelas de argumentos antes do switch. Alvos de efeitos registrados em77. Sem C++ alterado/build repetido. Próximo: scheduler e overrides de descoberta.

## 2026-10-25 — Tokenização dos eventos

Ciclo anterior foi progresso com UTF-8/CRLF. Exportados StringArrayStream e ParseInt; recuperados TAB preservando vazios e matrizes com contagens. inspect_event_table.py leu integralmente ambos os arquivos:228 registros,208 linhas de execução, timing0, sem matrizes null/sobras. Checks sintéticos de limites/erros passaram. Relatório76; comparação é estática, não runtime. Próximo: delegate, origem do contêiner e opcodes. Sem C++ alterado/build repetido.

## 2026-10-25 — UTF-8 e CRLF das tabelas

Ciclo anterior foi progresso ao ligar leitor de linhas. Resolvido Encoding.get_UTF8 e CheckUnityVersion6000 como condição de normalização CRLF. Script audit_game_table_text.py executado:44 arquivos válidos UTF-8, sem BOM,5103 pares CRLF. Evidência em75 e game-table-text-audit.json. Não remover CR genericamente: caminho Unity2022 deixa-o para tokenizador. Sem C++ alterado/build repetido. Próximo: tokenização/delegate.

## 2026-10-25 — Entrada textual de Data.Load

Ciclo anterior foi progresso com indexação por IDs. Ligada a cadeia Data.Load→JarInflater→LoadStringArray e nome literal Event.txt. Leitor usa LF, exclusão de segmentos vazios e flags sem trim/escape; há normalização condicional ainda não resolvida. BOM EFBBBF tratado por JString.New; decoder e delegate de registros pendentes. Evidências em74. Sem C++ alterado/testes repetidos. Próximo: resolver esses contratos antes de implementar loader fiel.

## 2026-10-25 — Indexação dos dados de gameplay

Ciclo anterior foi progresso ao localizar discrepância1133. Resolvido LoadData<EventData> e helper0x129fafc: máximo de IDs+1, lacunas null e sobrescrita na ordem. Modelo aplicado às tabelas locais produziu229 posições com lacuna180. Evidência em73/event-index-model.json. Ainda não fechada origem da sequência; sem declaração de observação runtime. Nenhum C++ alterado ou teste repetido. Próximo: seleção de recurso e delegate de construção.

## 2026-10-25 — Evento1133 e dados de eventos

Ciclo anterior foi progresso com inscrição do retorno. ExecEvent34376 e getter/loader de eventos exportados: índice fora do array retorna null, contador incrementa antes da checagem de script. Auditoria de dois Event.txt encontrou conteúdo idêntico,228 linhas, IDs0..228 sem1133. Comprimento do array efetivo ainda não comprovado; não declarar evento inexistente em runtime. Evidência em72/event-table-audit.json. Sem C++ alterado ou testes repetidos. Próximo: Data.Load e população dos dados de gameplay.

## 2026-10-25 — Assinante de QuestResult

Ciclo anterior foi progresso com estoque portado/testado10/10. Busca por parâmetros localizou TownSystem.OnReturnFromQuest; inscrição efetiva no evento31 confirmada por delegate e spec genérica. Handler dispara evento1133 para código1, atualiza score quando máximo de dano aumenta e marca flag de retorno. Helpers confirmam slot8 de SecureLong e flag estático+0x32. Evidência em71; sem C++ novo, teste repetido ou execução do jogo. Próximo: dispatcher1133, consumidor do flag e dados concretos de gameplay.

## 2026-10-25 — Concessões de inventário dos tesouros

Ciclo anterior foi progresso ao resolver OnFinish e QuestResult. Neste ciclo, cinco helpers de concessão, Find e ClampMax foram exportados; vtables confirmaram BaseData.Find nos quatro tipos. Descoberto teto1 de estoque com soma32 e ausência de piso. Regra portada em stage_rewards com testes de duplicata/descoberta/overflow; build Windows/CTest10/10 passou. Recruit grava1 e marca novidade incondicionalmente. Relatório70. Próximo: dados concretos/integração e assinantes do retorno. Sem execução de APK/save.

## 2026-10-25 — Finalização dos membros e payload de retorno

Ciclo anterior foi progresso ao recuperar guardas de saída. OnFinish e dois helpers exportados: cópia de nível/EXP ao cadastro precede destruição dos submembros e limpeza das listas. Setter limita nível1..1200; EXP é copiada sem clamp local. Resolvidas instanciações genéricas4001(MemberSystem) e2563(QuestResult), fechando coleção de EXP e payload do evento31. Evidência em69; sem mudanças C++, testes repetidos ou execução do jogo. Próximo: assinantes do retorno e concessões de inventário.

## 2026-10-25 — Guardas de resultado e saída

Ciclo anterior foi progresso ao localizar concessões e tipos numéricos. Neste ciclo, tabela de estados de StageSystem.Update, callback ReturnTown e callers diretos fecharam entrada normal no resultado: chefe+recompensa, estado3, pulso de entrada, resultado, estado4, ReturnTown0. Morte principal/tempo ligados a códigos1/2; diálogo de saída usa3. Segundo caller de resultado exige Config.DEBUG, resolvido por metadata/offset. Evidências em68 e stage-exit-evidence.json. Sem mudança C++ ou repetição de testes. Próximo: evento31, OnFinish e equipamentos.

## 2026-10-25 — Entrega no resultado da quest

Ciclo anterior foi progresso com contabilização portada e10/10 testes. Neste ciclo foram exportados quatro métodos de resultado/saída e quatro helpers de concessão. ShowQuestResult confirmado como consumidor do acumulador, com bônus de tesouros, crédito de moedas/cristais e EXP por entidade elegível. Diferença entre soma32 local e soma64 do saldo descoberta; custos de nível dependem de tabela/multiplicador. Relatório67 registra limites de alcance e estados de saída ainda desconhecidos. Sem C++ novo ou repetição de testes. Próximo: callers/callback de saída, inventário e wrappers.

## 2026-10-25 — Coleta e recompensas da fase

Ciclo anterior foi progresso: ordem de derrota e recompensas esclarecida. Exportados quatro métodos de coleta, com cobertura integral. Resolvidos campos/tabela de salto; animação termina após incremento para31 e tesouro de chefe tem rota direta. OnCollectedDrop acumula EXP/dinheiro, soma um prisioneiro independentemente do payload e preserva IDs/duplicatas. Portado esse estado em stage_rewards; sequência mista e overflow testados. Build Windows/CTest10/10 passou (stage-rewards-build.log). Evidência estática em66, sem executar APK. Próximo: concessão no encerramento da fase, guardas e cadência.

## 2026-10-25 — Derrota e recompensas

Ciclo anterior classificado como progresso: fatores de dano implementados e9/9 testes passaram. Neste ciclo, assembly de Defeat e seis helpers delimitaram ordem de mutações, contadores distintos, interpolação de EXP e seleção de tesouro por primeiro sucesso. Tabela de salto de CreateDrop comprovou que tipo3 força escala100. Evidência em65 e build/research/defeat-helpers; offsets em defeat-field-offsets.json. Confiança estática, sem execução do jogo. Nenhum C++ alterado ou teste repetido. Próximo: coleta, flags, RNG e prevenção de derrota duplicada.

## 2026-10-25 — Fatores Easing do combate

Pergunta: quais resultados numéricos os dois ajustes de Damage produzem? Assembly delimitado de Graph.Easing/FastMath.Sin/ToRadians e constantes ELF fecharam os casos de amplitude zero. Símbolo sin resolvido pela PLT; entradas finitas permitem retirar apenas a contribuição numérica nula. Resultado implementado em combat_rules com operações float32 separadas e wrap32 no bônus. Evidência e limites em64; confiança estática, sem oracle dinâmico.

Verificação: tools/verify_combat_easing.py conferiu148 pontos internos; build Windows e9/9 CTest passaram. Próximo: RNG/seed, guardas de colisão e Defeat/drops antes da integração de gameplay. Nenhum APK executado ou save acessado.

## 2026-10-25 — Dano acumulado e recordes

Turno anterior foi progresso ao ligar Damage a seus consumidores. Inscrição de OnDamage no evento15 confirmada via referências de método/genéricos. Handler soma D no total somente para receptor inimigo, sem calcular HP efetivamente removido. Reset em Start e atualização condicional do recorde da quest recuperados; nomes de campos confirmados por offsets. Relatório63.

Seis corpos exportados integralmente, busca delimitada de acessos registrada. Nenhum C++ alterado, teste repetido ou jogo executado. Próximo: callbacks/callers restantes e Easing antes de integrar dano; objetivo ativo com gameplay prioritário.

## 2026-10-25 — Do dano à derrota

Turno anterior foi progresso com afinidade portada. Agora retorno Damage confirmado como valor final<1, consultado após efeitos e evento15; getter inclui extraValue. Busca direta localizou três chamadas a Damage e duas a Defeat nos handlers de impacto/colisão. Callbacks podem influir no teste posterior; não antecipar decisão de morte. Relatório62.

Dois handlers exportados integralmente; FastMath.Sin exportado e constantes float lidas. Easing permanece dependente de fechamento numérico. Nenhum C++ alterado, teste repetido ou APK executado. Próximo: evento15/guardas/Easing antes de integração. Goal ativo, gameplay prioritário.

## 2026-10-25 — Valores concretos de afinidade

Turno anterior foi progresso com Parameter.Sub portado. Agora trecho do AppData..cctor fornece cinco pares e tabela de quatro taxas; referência InitializeArray resolvida via field-ref/default_data, SHA-256 do blob confere com nome do campo. Relatório61. Implementado estado explícito das tabelas e seletor; sem afirmar ausência de alterações posteriores.

Testadas25 combinações, sentinelas, precedência e índices inválidos. Build Windows/CTest9/9 passou. Nenhum APK executado. Próximo: Easing e efeitos do dano antes de integrar combate completo. Goal ativo, gameplay prioritário.

## 2026-10-25 — Modificadores e subtração de valor

Turno anterior foi progresso com limiar de acerto portado. Damage após acerto foi seguido até Parameter.Sub: ordem dos fatores, mínimo aleatório e guarda DEBUG identificados. Afinidade usa tabela de pares e tabela de taxas; valores ainda pendentes. Parameter.Sub e IntExtension.Clamp comprovados por assembly/offsets, sem uso de extras e com máximo0 desabilitando clamp. Relatório60.

Implementado subtract_parameter no núcleo C++17; testes ampliados para excesso, sinal, máximo0/negativo e wrap32. Build Windows e CTest9/9 passaram. Nenhum APK executado. Próximo: tabelas/Easing/efeitos pós-dano, antes da integração; goal ativo com gameplay prioritário.

## 2026-10-25 — Regra de acerto implementada

Turno anterior foi progresso na entrada do combate. Agora assinatura de Damage e ramos de parâmetros7/9 conferidos; limiar base90/80+clamp da diferença recuperado. Random.Hits consome Rand(100) sem curto-circuito por limiar; wrappers mostram resto/negação de32 bits, JRandom escolhe backend por configuração. Sem algoritmo de RNG presumido. Relatório59.

Implementado hit_threshold C++17 independente, com testes de limites, sinal e overflow. Build Windows e CTest9/9 passaram. Nenhum jogo executado ou save acessado. Próximo: defesa, afinidade, modificadores e aplicação de HP, mantendo gameplay prioritário. Goal ativo.

## 2026-10-25 — Prioridade alterada para gameplay

Turno anterior confirmou a preferência do usuário, sem avanço técnico; retomada agora com exports e leitura concreta de combate. BattleSystem.Update apenas enumera seu subset no corpo observado. Intervalo humano depende de total de parâmetro6 e mínimo2; inimigo usa parâmetro6 diretamente, mínimo1 e resto de campo do componente. FPS identificado via offsets. Cadeia Damage/Defeat inventariada com RNG, afinidade, redução de parâmetro, experiência e drops; fórmulas completas pendentes. Relatório58.

Quatorze FDEs exportados integralmente; chamadas de três métodos inventariadas. Nenhum C++ alterado ou teste repetido. Saves/tradução ficam em segundo plano. Próximo: origem dos parâmetros/contadores e cálculo de dano, preservando ordem do RNG. Goal ativo.

## 2026-10-25 — Inicialização da flag e tradução por chave

Turno anterior foi progresso: framing/encoding de strings documentados. Config..cctor zera bytes0x51–0x54, incluindo SAVEDATA_LOCALIZE; Config.Create chama FieldInfo.SetValue, portanto não presumir flag permanentemente false. ReadTranslate usa TranslateEnable próprio, comprimento<=127, regex de não dígitos e exclusão por prefixos, normaliza CRLF/LF/TAB/backspace para chave e restaura escapes somente no valor encontrado. Relatório57.

Três FDEs/7.484 bytes exportados integralmente; leitura de tradutor reaproveita evidência anterior. Config.Init exportado ainda não totalmente interpretado. Nenhum C++ alterado, teste repetido ou save acessado. Próximo: origem da configuração/tabela, WriteTranslate e detector; objetivo ativo.

## 2026-10-25 — Formato das strings e ramificações de encoding

Turno anterior foi progresso: bloco2 e campos DelayEvent comprovados. ReadString/WriteString e helpers exportados; escritor usa UTF-8 e comprimento BE em bytes, null geral=-1, vazio=0. Leitor chama detector GetCode com candidatos iniciais UTF-8/Shift_JIS e pode chamar ReadTranslate. Flag resolvida como Config.SAVEDATA_LOCALIZE via metadata/tabela de offsets. WriteTranslate retorna void, não substitui referência no caller. Relatório56 separa fatos e ramos ainda não interpretados.

Onze FDEs/7.100 bytes com cobertura sintática integral; sem codec de string aproximado, mudança C++, repetição de testes ou acesso a saves. Próximo: flag e tradutores/detector; goal ativo, investigação estática útil disponível.

## 2026-10-25 — Bloco de eventos adiados

Turno anterior foi progresso: codec54 implementado/testado. Busca direta BL/B não encontrou callers desse par de métodos; inferência de integração ficou explicitamente aberta. DelayEvent.Serialize/Deserialize têm callers em PackSave/LoadGame, com seletor/elemento2 confirmado. Layout de campos resolvido por tabela nativa de offsets; defaults e string vazia conferidos nos bytes/metadata. Relatório55.

Quatro FDEs/1.068 bytes exportados integralmente; corpos mostram cinco inteiros BE, string por helper ainda não examinado e comprimento/elementos de array. Null vira vazio; leitura pode deixar mutações parciais. Nenhum codec especulativo de string, C++ alterado, teste repetido ou save acessado. Próximo: helpers de string e consumidor temporal; goal ativo.

## 2026-10-25 — Serialização das linhas de scripts

Turno anterior foi progresso: MakeArray e consumidor documentados. Agora SerializeScripts/DeserializeScripts foram lidos e WriteInt confirmado como BE; formato quantidade/comprimento/inteiros recuperado. Quantidade externa negativa retorna vetor vazio, sem equivalência com comprimento negativo interno. Codec C++17 de buffer finito implementado, com limites de snapshot, falha e consumo documentados em54.

Validação: fixture explícita, extremos assinados, todos os truncamentos da fixture, sufixo, contagens negativas/enormes; build Windows e CTest8/8 passaram. DelayEvent construtor exportado mostra campos e cópia de array, mas semântica e persistência continuam pendentes. Nenhum save acessado ou APK executado. Próximo: callers do bloco e execução adiada; goal ativo.

## 2026-10-25 — MakeArray e consumidor dos grupos

Turno anterior respondeu ao resumo solicitado, sem novo avanço de engenharia; retomada por ação estática disponível. Resolveu-se a instanciação MakeArray para BaseData[][] e a construção recursiva de cinco linhas vazias com[5,0]. Consumidor em NewGame usa slots16/15, CheckNotFind/CheckTerm; ambos retornam zero nos22 tipos auditados. Busca delimitada por offsets preservou falsos positivos de vtable/estáticos como candidatos, sem alegar ausência global de consumidores. Relatório53.

Exports adicionais incluem executor e persistência de scripts para próximo recorte. Nenhum C++ alterado, APK executado ou save acessado. Ainda não há comprovação de limpeza global pré-carga. Goal ativo; próximo: opcodes e execução adiada, mantendo investigação estática.

## 2026-10-25 — Descritores concretos e GetTiming

Turno anterior classificado como progresso: overrides documentados em51. Agora GetDataList34321 identifica dez arrays e seletores11/0/7/8/9/10/14/15/16/17 em ordem; tipos de retorno resolvidos via metadata e relocations. InitCheckScriptData consulta slot14, resolvido como BaseData.GetTiming retornando zero nos22 tipos auditados; final preenche array+0x2e8 com10000 via Util.ArrayInit. Relatório52 registra contratos e limitações.

Validação:14 novos FDEs/2.988 bytes com cobertura sintática integral; releitura do export de InitCheckScriptData. Nenhum C++ alterado, teste repetido, APK executado ou save acessado. Próximos alvos: MakeArray e população/consumidores das tabelas; limpeza ECS continua aberta. Goal ativo, avanço estático disponível.

## 2026-10-25 — Overrides de preparação dos dados

Pergunta: chamadas virtuais de NewData escondem limpeza do World? Método: caminhada de tipos derivados de BaseData, decodificação de slots18/8 e exports ARM64 limitados por FDE. Resultado:22 tipos,10 implementações NewGame; Find da base escreve estado/booleano, override QuestData só altera estado se zero. AreaData.oldNewGame recria arrays; QuestRecord construtor encaminha a Object. Relatório51 e artefatos locais registram endereços e limites.

Validação:14 FDEs/1.556 bytes integralmente cobertos; associação das chamadas com metadata. Não prova todos os objetos de GetDataList nem ausência de efeitos indiretos. Nenhum C++ alterado, teste de runtime alegado ou save acessado. Próximo: descritores concretos e InitCheckScriptData; goal ativo, sem bloqueio dinâmico.

## 2026-10-25 — Destroy e título

Turno anterior: progresso por cadeia ContinueGame/NewGame/NewData confirmada. World.Destroy agora exportado: coleção, entidades, sistemas, eventos; sem store direto em World+0x38. Busca BL/B por Destroy retornou vazio, registrada como cobertura limitada. Init/Finish do título não chamam limpeza ECS diretamente; efeitos transitivos ainda desconhecidos. Três FDEs/876 bytes em50.

Nenhum C++ alterado ou teste repetido. Próximo: referências indiretas e descritores/overrides, preservando preparação do mundo como lacuna. Goal ativo; ainda há investigação estática útil.

## 2026-10-25 — Preparação no fluxo Continuar

Turno anterior: progresso por InitSystemSaveData/BaseData resolvidos. Busca direta BL/B localizou TitleForm.ContinueGame→LoadGame; o ramo passa antes por NewGame e depois por InitWorld e SelectForm. NewGame chama SaveSystem, recria SecureProperty e passa por NewData. Este despacha BaseData.NewGame(slot18) sobre GetDataList e termina em InitCheckScriptData. Relatório49 registra evidências e limitações; limpeza ECS ainda depende de tipos concretos/transições anteriores.

Nenhum C++ alterado, teste repetido, save acessado ou rotina original executada. Próximo: GetDataList/overrides e callers de limpeza do World. Goal ativo, sem dependência de teste dinâmico.

## 2026-10-25 — Inicialização de SecureProperty e dados anteriores ao ECS

Turno anterior: progresso por EOF concreto resolvido. Export de InitSystemSaveData568 bytes confirma criação de SecureProperty em+0x190 e propriedades condicionais, sem reset direto de World. Referência de array em DeserializeData resolvida como BaseData[]; slots identificados como Deserialize/Deserialized/DeserializeShare. Caminho LoadGame usa modo normal antes do ECS. Comprimentos e índices assinados, caminho inválido e ausência de substream limitada registrados em48.

Nenhum C++ alterado, teste repetido ou save aberto. Preparação do World ainda pendente; próximos alvos são tabela de descritores, overrides e callers de LoadGame, não exigência de teste dinâmico. Goal ativo.

## 2026-10-25 — EOF do stream concreto

Turno anterior: progresso com ordem Dictionary e callers do save. Estado atual conferido; nesta rodada a vtable de ByteArrayInputStream resolveu Read herdado de InputStream. Construtor aloca MemoryStream, cujo Read retorna0 no EOF, convertido para-1 pelo wrapper. Isso fecha leitura incompleta como erro de StreamUtil no caminho de LoadGame, sem generalizar o loop de zero persistente de42. Relatório47 e atualização explícita do relatório anterior.

LoadSystem/DeserializeData exportados e parcialmente examinados; a preparação do mundo permanece pendente, não confundida com EOF. Nenhum C++ alterado, APK executado ou save lido. Próximo: inicialização e campos carregados antes de World.Deserialize. Goal ativo.

## 2026-10-25 — Ordem do registro e callers do save

Turno anterior: progresso com hooks e estrutura de escrita45. MoveNext de Dictionary.Values agora confirmado: ordem crescente por entradas ativas, versão por enumerador, sem snapshot global entre passagens. Busca BL/B com owners limitados por FDE localizou AppData.PackSave e LoadGame como callers diretos do World. LoadGame usa elemento13 de SecureProperty para ByteArrayInputStream; PackSave seletor13 grava World. World escreve entidades antes de sistemas.

Relatório46 e JSON local de callers preservam escopo/limitações. Preparação por LoadSystem/DeserializeData e EOF do stream concreto continuam pendentes; não presumida limpeza ou travamento. Nenhum C++ alterado, save aberto ou teste repetido. Goal ativo; próximos alvos estáticos definidos.

## 2026-10-25 — Hooks e estrutura de escrita das entidades

Turno anterior: progresso com codec Seb/layers e CTest7/7. Nesta rodada, vtables dos três componentes e corpos RET fecharam ausência de payload extra/alteração em hooks herdados. SerializeComponents escreve quantidade de slots, presença e OnBeforeSerialize→Serialize→Extra por posição. EntityManager escreve contador/quantidade, cabeçalhos em uma passagem e componentes em outra enumeração de Dictionary.Values. Entity.Serialize confirma ID32/flag8.

Relatório45 registra estrutura, limites e ordem. Não ordenar ou intercalar entidades por convenção; Dictionary/efeitos de hooks ainda pendentes. Nenhum C++ alterado ou teste repetido; nenhum save real aberto. Goal ativo, próximo passo em factory, ordem do registro e callers do save.

## 2026-10-25 — Payload variável de sprites e layers

Turno anterior: progresso com endianness e codecs fixos/CTest7/7. Código atual e evidências42/43 conferidos antes de acrescentar SpritePayloadState, encode/decode do cabeçalho e layers. Null e vazio preservados; leitura127 aceita, escrita127 rejeita; demais comprimentos negativos falham. Decoder retorna consumo de prefixo, adequado para futura composição.

Build Windows/CTest7/7 passou com testes adicionais de bytes conhecidos, truncamento em todos os prefixos, comprimento0–127, negativos, sentinela e trailing bytes. As decisões de erro de buffer finito estão documentadas como fronteira diferente de StreamUtil, sem simular travamento ou stores parciais. Relatório44. Próximo: hooks de componentes e envelope/stream concreto; goal ativo, nenhum input dinâmico necessário.

## 2026-10-25 — Ordem dos bytes e codecs fixos

Turno anterior: progresso com campos persistidos e comportamento de StreamUtil. Sete FDEs adicionais confirmaram BitConverter STRH/STR little-endian seguido de inversão na escrita com flag1, resultando em payload big-endian. Short preserva16 bits baixos sem checagem de faixa. Codecs fixos AnimationPlayback/DirectionState adicionados, sem simular stream parcial ou save completo.

Build Windows/CTest7/7 passou. Teste novo cobre fixtures de bytes, extremos32, truncamento,65536 padrões de short e256 valores de direção/booleano, com comparação numérica independente. Relatório43 contém evidência e limites. Próximo: Seb/layers e integração incremental de leitura; goal ativo, nenhum save real ou APK executado.

## 2026-10-25 — Payloads de componentes e StreamUtil

Turno anterior: progresso por cadeia de destruição/desserialização confirmada. Nesta rodada,18 FDEs adicionais revelaram campos persistidos Animation/Direction/Seb, extensão de sinal8/16 bits, layers com comprimento byte<127 e sentinela null-1. ReadBoolean aceita somente1. StreamUtil.Read repete sem progresso quando recebe zero antes de atingir a quantidade; caso registrado como evidência estática, sem provocar travamento no original.

Relatório42 preserva lacunas de endianness/conversores e distinção entre codec de payload e save completo. Nenhum C++ alterado ou save real lido. Próximo: EncodeUtil e stream concreto para implementar payloads sem aproximações. Goal ativo; nenhuma dependência dinâmica necessária neste ponto.

## 2026-10-25 — Destruição e duas passagens do carregamento

Turno anterior classificado como progresso por comparador/enumeração confirmados. Nesta rodada, assembly/FDE/vtables e referências de Dictionary fecharam DestroyEntity/DestroyAll: RemoveAll precede DisposeEvents, registro removido condicionalmente e limpeza global após enumeração. EntityManager.Deserialize carrega contador, cria cada entidade, lê ID/flag, registra callbacks e só depois carrega componentes em segunda passagem. Byte de seletor/limite sofre extensão de sinal. Relatório41 registra detalhes e lacunas.

Nenhum save aberto, APK executado ou C++ alterado. Próximo: wrapper Destroy, StreamUtil e serialização de Animation/Seb/Direction, além da preparação dos callers. Goal ativo; ainda não depende de testes dinâmicos.

## 2026-10-25 — Comparador e enumerador efetivo da animação

Turno anterior: progresso com resolução genérica, predicado nativo e CTest6/6. Nesta rodada,10 novos FDEs/1092 bytes confirmam comparador por identidade de referência, hash entity+0x28 mascarado e tratamento interno de null. GOTs de AnimationSystem resolvem GetEnumerator/MoveNext/Dispose de HashSet<Entity>. MoveNext verifica versão e percorre slots ativos crescentes; Add reusa slots removidos e crescimento preserva posições. Exemplo A,B,C→remove B→add D resulta estaticamente em ordem A,D,C.

Relatório40 documenta evidências e distingue inferência estática de captura runtime. Nenhum C++ alterado, sem teste repetido. Próximo: Initialize/capacidades, escritores de IDs e limpeza antes de integrar conjunto fiel. Goal ativo, caminhos estáticos disponíveis.

## 2026-10-25 — Coleções genéricas e predicado do filtro

Turno anterior: progresso com ContainsAll/Any, testes e relatório38. Nesta rodada, referências genéricas resolvidas por tabelas do ELF e metadata: HashSet<Entity> para grupos e IsNullOrEmpty<int> para critérios. Sete novos FDEs,1496 bytes, mostram duplicatas sem inserção/versionamento, remoção condicional e wrappers que substituem referências de critérios. Notificação de Subset.Add continua possível em duplicata, porque ignora o resultado do conjunto.

Predicado completo AND/OR/NOR adicionado ao núcleo, mantendo curto-circuito, null e vazio. Sem aproximação de HashSet, hash de Filter ou ordem do Update. Build Windows/CTest6/6 passou. Relatório39 registra endereços, índices e limites. Comparador e enumerador são próximos alvos estáticos; goal ativo, sem APK executado.

## 2026-10-25 — Predicados de componentes e acesso aos grupos

Turno anterior: progresso por fechamento da fábrica concreta e callbacks, relatório37. Agora13 novos FDEs e slots10/11 de BaseEntity confirmam ContainsAll/Any, inclusive listas vazias e curto-circuito que determina quais índices inválidos são alcançados. Predicados adicionados a ComponentSlots com testes; build Windows/CTest5/5 passou. Relatório38.

Filter.Matches ordena AND/OR/NOR e rejeita entidade null. Helper genérico que decide pular listas foi delimitado: null retorna true, não nulo consulta método0 de interface contextual e compara com zero. Identidade desse contexto ainda pendente, sem transformar hipótese de coleção vazia em implementação. Subset.get_entities retorna diretamente+0x30; overload Add(entidade) não notifica, diferente de Add com dados do componente. Próximo: resolver contexto e coleções/enumeração para integrar ECS fielmente. Goal ativo; nenhum Android executado ou input necessário.

## 2026-10-25 — Fábrica concreta e grupos de componentes

Turno anterior classificado como progresso: módulo ComponentSlots, testes5/5 e relatórios35/36 publicados. Nesta rodada,13 FDEs adicionais, relocação ELF e vtables fecharam o caminho World.cctor→MyEntityManager(35)→Create→BaseEntity para seletor0. Outros seletores retornam null na fábrica examinada. AddEntity inscreve os três callbacks mesmo quando pula o registro inicial na coleção.

Handlers de adição e remoção ambos chamam Subset.Add e, se false, Subset.Remove. A interface do filtro foi resolvida como IFilter.Matches; Change apenas encaminha notificação nesse nível. Invalidação após remoção acontece depois do callback, diferente da adição. Coleção genérica, cache e filtros concretos ainda precisam de investigação; nenhuma regra aproximada foi implementada. Evidências e limites em37-entity-factory-and-subsets.md. Nenhum C++ alterado, sem repetição de build ou execução Android. Goal ativo, próximo passo estático definido.

## 2026-10-25 — Componentes concretos, pausa e carregamento

Pergunta: adicionar componente duplicado substitui o anterior? Método:14 FDEs de BaseEntity, notificações e factories, com cobertura sintática integral e inspeção de assembly/metadata. Add ignora posição ocupada; Change substitui e notifica mesmo com ponteiro igual ou null. Remoção altera antes de notificar; RemoveAll varre posições vivas em ordem crescente. A fábrica de EntityManager continua virtual, portanto o contrato não foi generalizado a toda entidade.

Implementação: módulo C++17 ComponentSlots com referências emprestadas, sem presumir pool/propriedade. Revisão independente confirmou o escopo; testes exercitam inclusive callbacks que reinserem componentes e lançam exceções. Build Windows e CTest5/5 passaram. Relatório35 registra endereços, limites e reprodução. Linux e execução original não testados.

Investigação complementar: quatro FDEs,216 bytes, resolvem hooks BasePauseSystem e World.Deserialize; interface de pausa identificada por relocação/metadata como IPausable. Entidades são desserializadas antes dos sistemas, e o byte World+0x38 só é marcado depois. Preparação do estado de entrada permanece pendente; não inserir limpeza presumida. Relatório36.

Próximo: fábrica concreta, callbacks de grupos e EntityManager.Deserialize. Goal ativo; há investigação estática útil e nenhum teste dinâmico é solicitado neste ponto.

## 2026-10-25 — bootstrap nativo e catálogo IL2CPP

- Pergunta: o que NativeLoader.load realmente carrega e onde procurar a lógica do jogo?
- Método: extração seletiva somente de libmain ARM64; Ghidra 12.1.4 headless direto; exportação de funções, assembly e tabela JNINativeMethod; comparação de slots JNI com header local JDK. Pesquisa independente dos metadados v31 por parser com schema Il2CppDumper fixado em commit.
- Achados: libmain registra NativeLoader.load/unload e carrega libunity/libil2cpp; nove funções não-thunk exportadas. Catálogo: 64 images, 8.619 tipos e 63.273 métodos; main.Main IL2CPP tem OnCreate/OnUpdate/OnDraw, distinto de main.Main DEX.
- Evidências: `05-il2cpp-metadata.md`, `06-native-bootstrap.md`; hashes, índices, endereços e offsets nos relatórios; artefatos brutos em research/local e build/research, ignorados.
- Confiança: confirmed para relações/tabelas/chamadas estáticas observadas; probable para atribuição de subsistemas por nomes; unknown para regras, métodos nativos correspondentes e runtime.
- Validação: Ghidra import/analysis/export exit0; nove decompilações completas; tabela JNI concorda com protótipos DEX; extração byte-idêntica ao ZIP; traversal rejeitado; parser metadata determinístico e truncamentos inválidos rejeitados; hash APK inalterado. Não executado o jogo nem compilado o port.
- Próximo: mapear registros/módulos IL2CPP para localizar corpos dos três callbacks de main.Main.

## 2026-10-25 — bootstrap, loop Android e input

- Pergunta: como a activity alcança o runtime e encaminha eventos?
- Método: inventários de membros e inspeções focadas via REA/JADX; overloads selecionados pelos protótipos. Relatório e índice de evidências em `04-android-startup.md`.
- Achados: construtor UnityPlayer prepara bibliotecas e JNI, cria view e inicia U0; U0 usa Looper/Handler T0; handler chama nativeRender e agenda mensagens. Encaminhamento Android de toque identificado; não equivale a mapear regras de gameplay.
- Confiança: confirmed para chamadas recuperadas sem warning; probable para detalhes do handler que contém aviso PHI/duplicação. Timing, primeira cena e lógica IL2CPP seguem unknown.
- Incidente: consultas paralelas com JVMs de heap ~1.980 MiB provocaram commit_memory error 1455, unreadable_output e cleanup_incomplete. Nenhum java.exe remanescente observado; CLI pendentes interrompidas. Wrapper passou a usar 768 MiB por padrão e consultas sequenciais terminaram com sucesso. Arquivos de erro preservados localmente, sem confundi-los com evidências bem-sucedidas.
- Validação: processos de análise bem-sucedidos retornaram código 0; envelopes incluem hash do APK, engine e IDs. Não houve execução do APK ou build do port.
- Próximo passo: conferência smali dos ramos e investigação da fronteira JNI/IL2CPP.

## 2026-10-25 — acesso do agente resolvido e primeiro método

- Mudança de contexto: usuário disponibilizou ambiente com permissões irrestritas; repetição autorizada da análise passou sem alterações de ACL, política PowerShell ou instalação.
- `inspect_android_class(main.Main)` concluído: quatro métodos (`<init>`, `onActivityResult`, `onCreate`, `onRequestPermissionsResult`), nenhum campo ou classe interna reportado. Evidence `ev_640553a3d2f77b58688658a11db57fe978fe7120f24eb9729e20b19e666cace6`; arquivo local `research/local/main-class.json`.
- `inspect_android_method(main.Main, onCreate, overload 0)` retornou Java completo sem fallback. Evidence `ev_58d6ca4d7212dc3172239c3b19e129b199d80c0e0247e8b53a5d468abd132996`; arquivo local `research/local/main-oncreate.json`.
- Observação estática: `main.Main.onCreate(Bundle)` chama `super.onCreate(bundle)` e depois `plugin.Utility.CreateNotificationChannelIfNeeded()`. JADX associa o override a `com.unity3d.player.UnityPlayerActivity`. Isso orienta a próxima inspeção; não demonstra execução ou primeira cena.
- Validação do wrapper `tools/rea-local.cmd`: comando real de decompilação terminou com código 0. Nenhuma alteração global do ambiente; MCP permanece separado da CLI funcional.
- Próximo passo: inspecionar `UnityPlayerActivity.onCreate` e suas dependências de inicialização.

## Validação JADX — saída fornecida pelo usuário

- Fonte: saída completa de `inspect_android_package` colada na conversa; não foi uma execução bem-sucedida no ambiente do agente. Evidence `ev_7f9485b41b5082c6852c0e61af5ac83bc155559a950e769aabca4395c699090a`.
- JADX headless MCP 0.7.1 carregou o APK (`LOADED`), reportou 12.499 classes e 1.399 recursos, com manifesto `complete`. SHA-256 do APK e do JAR coincidem com os registrados.
- Comparação: pacote, versão 1.4.0/140, SDK 24/35, 15 permissões e launcher `main.Main` concordam com o parser local. JADX resolve orientação como `portrait`, launchMode como `singleTask` e referências de recursos em nomes.
- Diferença de representação: nomes de classes internas aparecem com ponto no manifesto JADX (por exemplo `ClockTime.ClockReceiver`), enquanto o parser local preserva `$` (`ClockTime$ClockReceiver`). Manter identificadores brutos; nomes de exibição JADX podem ser aliases, conforme as limitações do provider.
- Confiança: confirmed para as observações estáticas fornecidas. Sem execução do jogo, verificação de assinatura ou decompilação de métodos demonstradas por esse resultado.
- Próxima ação: `inspect-android-class` para `main.Main`, seguido de inspeção dos métodos de inicialização. CLI do usuário funcional; bloqueio Win32 5 observado no agente permanece separado.

## 2026-10-25 — identidade e composição do APK

- Pergunta: qual artefato foi fornecido e que rota de análise ele exige?
- Método: leitura de AGENTS.md e skill REA; descoberta de ferramentas; SHA-256 PowerShell/REA; grafo de arquivo REA; leitura ZIP, AXML e ELF com Python padrão.
- Achados: versão 1.4.0/140, três DEX, doze bibliotecas ARM/AArch64, Unity/IL2CPP, 1.184 assets; duas colisões de caixa. Evidências e localizadores em `01-apk-inventory.md` e `02-architecture.md`.
- Confiança: confirmed para presença/declarações; probable para localização predominante da lógica em IL2CPP; unknown para funcionamento offline, métodos de gameplay e viabilidade do port.
- Validação: JSON determinístico, seis truncamentos AXML rejeitados, CRC ZIP integral válido, SHA-256 preservado. Nenhuma execução do jogo.
- Limites: parser local de manifesto aguarda comparação independente; assinatura não verificada. REA Android sem JAR JADX; apktool e ADB indisponíveis na sessão.
- Próximo passo: validar manifesto em ferramenta independente e mapear somente `main.Main` até a inicialização Unity.

## 2026-10-25 — preparação do projeto e ferramentas

- Pergunta: como deixar o trabalho retomável sem antecipar a implementação?
- Método: estrutura de diretórios, documentação, exclusão de dados locais do Git e scripts de instalação local.
- Achados: JDK 21 e Ghidra 12.1.4 existentes; ausência no PATH não significa ausência no disco. Provider Ghidra REA Windows limita alvos suportados: não assumir suporte ARM ELF pelo bridge.
- Instalação autorizada pelo usuário. Acesso HTTPS ao GitHub via curl falhou com código 7; ferramentas não foram baixadas. Scripts de preparação não equivalem a instalação concluída.
- Próximo passo: executar instalação em ambiente com rede e política compatível, validar cada ferramenta e registrar os resultados em `.tools/installed-tools.json`.


## 2026-10-25 — Callbacks ARM64, limites e primeiras funções pequenas

Pergunta: quais subsistemas são chamados por main.Main e quais valores são comprováveis?
Método: mapa de tokens/relocations, 137.201 FDEs via pyelftools, Ghidra direto e leitura independente dos opcodes de transferência ARM64.
Resultado: três callbacks delimitados; 265 transferências inventariadas. SetViewport armazena 240/280; GetTargetFps retorna 120 durante IsFormChanging, senão usa formulário/fallback; SetJingle alcança SoundPlayer.SetVolume. Detalhes e VAs em `09-native-callbacks.md`.
Validação: todos os 245 BL exportados pelo Ghidra coincidem com bytes; mais dez BL detectados em lacunas de exportação ficam explicitamente sem segundo disassembler. Três tentativas de pseudocódigo OnUpdate inconsistentes; descartado como base de regras, preservado localmente.
Confiança: confirmed para instruções e correspondências de tabela; runtime e significado de campos unknown.
Ferramentas: apktool/ADB locais funcionais; adb devices vazio, nenhum emulador encontrado. Nenhum APK instalado ou executado.
Próxima ação: catálogo Unity, leitores de recursos, slot virtual de desenho e cenário de observação Android.

## 2026-10-25 — Leitor de recursos e pausa por instrução da skill

Pergunta: qual caminho lê os TextAssets opacos?
Método: quatro métodos AssetReader e seis sobrecargas Encode/Decode, selecionados por metadata e FDE, em Ghidra headless sequencial.
Resultado: GetData chama Decode sob condições não resolvidas; Decode usa um array estático como chave e alcança rotina XOR periódica. Não foi recuperada chave nem decodificado conteúdo. Detalhes e endereços em `11-resource-reader.md`.
Confiança: confirmed para chamadas e instruções; aplicação aos 30 TextAssets de prefixo 65b6 continua unknown.
Limitação de workflow: skill asset-extraction exige parar ao encontrar assets que precisam de chave; usuário será informado para definir próximo caminho. A pesquisa não concluiu que se trate de controle de licença.
Validação: scanner ARM64 repetido com saída idêntica; identidade ELF incorreta rejeitada sem criar saída. Exportação de recursos teve uma tentativa rejeitada pelo lock de projeto ocupado; repetição sequencial concluiu sem remover locks.
Próxima ação: aguardar escolha entre análise sem decodificação e preparação de observação Android, conforme pedido do usuário para parar quando houver decisão necessária.

## 2026-10-25 — Retomada autorizada e extração validada

Usuário autorizou explicitamente investigar a chave e extrair os assets cifrados. Essa instrução substitui a restrição da skill registrada na entrada anterior.
Pergunta: como obter dados utilizáveis a partir dos 30 TextAssets de prefixo 65b6?
Método: xrefs ARM64 para SetEncryptionKey, análise AppData.Init e inicializador do metadata, XOR periódico observado no código; leitor JarInflater._load e round trip de contêineres.
Resultado: chave de 44 bytes obtida da própria cópia, sem chave incorporada ao script; extraídos 38 TextAssets e 1.963 membros, incluindo 973 PNGs. Formato, offsets, evidências e limites em `12-asset-extraction.md`.
Validação: 30 reconstruções byte a byte, XOR inverso idêntico, 973 PNGs verificados e totalmente decodificados, título inspecionado visualmente; seis testes sintéticos passaram. Extração repetida e `--check` passaram por Python e uv. Arquivos locais em `data/extracted`; APK não alterado.
Confiança: confirmed para esta versão e esses contêineres; sem afirmação de renderização ou comportamento runtime. Áudio/fontes Unity e semântica de animações ainda pendentes.
Próxima ação: mapear INF/SEB e leitores nativos para apresentação fiel, antes de criar renderer.

## 2026-10-25 — Recursos Unity e estrutura SEB

Método: UnityPy 1.25.4/fmod_toolkit 0.1.3, validação de FSB/WAV, FreeType, PNGs e assembly dos leitores SEB.
Resultado: opção `--unity-media` integrada ao extrator, com 68 PNGs e buffers originais, 23 pares FSB/WAV, cinco fontes, cinco registros de textura vazios. `--check` de `data/full` passou; WAV/fontes coincidem por hash com protótipo independente. Uma fonte do motor apresenta checksums incorretos já no original; preservada.
SEB: 303 arquivos consumidos integralmente e reconstruídos a partir de escalares; 452 grupos e 2.686 registros, modos 0/1. Doze testes Python dos dois leitores passaram. Nenhuma interpretação de timing foi assumida. Relatórios 13/14.
Ferramentas nativas: Visual Studio instalado fora PATH contém MSVC19.51.36252.0, CMake4.3.1-msvc1 e Ninja1.13.2; smoke C++17 x64 passou sem instalar nada. Leitor C++ portátil de contêiner em preparação, com diferencial contra os30 arquivos locais.
Próxima ação: semântica de sprites, leitor nativo e seleção da implementação RNG; comparação com Android ainda pendente.

## 2026-10-25 — Leitores nativos, OPT e visualizador técnico

Método: implementação C++17 dos formatos comprovados, CMake/CTest Windows, diferenciais contra Python; SDL3/stb fixados para captura headless e comparação RGBA.
Resultado: JarInflater nativo concorda nos30 contêineres/1963 membros; OPT nativo concorda nos286 arquivos/2559 células/2481 peças. Caminhos Unicode Windows passaram. Build padrão não exige SDL.
Visualizador opcional SDL3 3.4.18/stb capturou fixture com alpha e título extraído com igualdade exata de pixels. Nenhuma janela interativa foi aberta ou cena do jogo reconstruída. Relatórios15/19.
OPT: parser Python e encoder passaram286 round trips; harness gera visualização dos limites ocupados, verificando2481 peças por recorte reverso.18 arquivos excedem a grade de header e ficam sinalizados; dimensões originais permanecem desconhecidas. Vinte testes Python passaram (contêiner, SEB, OPT/harness). Relatório18.
RNG: JRandom constrói três implementações e seleciona por campo Logic; wrappers de módulo e NextLong não equivalem a substitutos padrão C++. Vtables corroboradas, seleção efetiva/seed runtime ainda desconhecidas. Relatório17.
Próxima ação: dimensão lógica de Image, cadeia de recursos, SEB nativo e comparação futura com original. Goal segue ativo.

## 2026-10-25 — Continuidade estática solicitada pelo usuário

Pergunta: o que pode avançar sem executar o original? O usuário adiou testes dinâmicos e solicitou esgotar caminhos estáticos úteis antes de voltar a pedi-los.

Método: auditoria com hashes de todos os membros extraídos, preservação de registros INF e comparação de candidatos; assembly dentro de FDEs para filtros de callbacks, pausa/foco e encerramento. Não houve execução do APK.

Achados: 51 catálogos/1.298 registros, um ID repetido, três linhas sem ID, 21 referências com candidato PNG em vez do nome literal, três referências ausentes em room com nomes encontrados em shop. A política de substituição e fallback ainda depende de código. Relatório25 e saída local reproduzível. Os23 testes Python passaram; repetição do relatório idêntica após fixar saída LF.

Loop: TotalFrames pode avançar durante splash sem Main.OnUpdate; condições de pausa/encerramento também pulam o hook. OnGUI tem filtros por evento, resolução e caminhos que desenham splash/captura; a espera por FPS é condicional e inclui tarefas UI, não equivale a timestep fixo. Quatro novos FDEs com cobertura sintática integral; detalhes, endereços e limites no relatório24.

ReDownload: duas sobrecargas AppData contêm apenas RET X30, conferido em bytes e FDE, sem concluir completude ou offline. Confiança: confirmed para estrutura/destinos e dados citados; unknown para sequência/frequência reais e políticas ainda não rastreadas.

Próxima ação: resolução de aliases/idioma, política de FPS, avanço SEB e serializer. O ambiente Android pendente não impede esses passos.

Continuação estática: Config.ConvertExtension troca ocorrências `.gif→.png` e `.mld→.ogg` com seletor 2/3/4; inicializador começa em2. JarInflater normaliza nomes e consulta candidatos de AssetReader, incluindo idioma. O vínculo room/shop continua desconhecido; relatório26.

Política de taxa: MyConfig inicializa30, FormManager copia para alvo padrão; GetTargetFps prioriza transição120, depois formulário e padrão. Main.OnUpdate contém overrides condicionais120/400/2 e aplica o getter ao Unity com vSync0. FormManagerBase._execute pode produzir múltiplos eventos lógicos por passagem; não equiparar30 a timestep de gameplay. Config+0x63, habilitador da espera manual, começa zero. Relatório27 distingue inicializações, mutações ainda não inventariadas e frequência não observada. Novos exports por FDE têm cobertura sintática integral; nenhuma execução do original. Próxima investigação: eventos de _execute e precedência de idioma, ambos estáticos.

## 2026-10-25 — Despacho lógico e animações

Pergunta: qual evento atualiza formulários e como o avanço SEB se relaciona ao desenho? Método: assembly por FDE, vtables metadata, tabela de salto e constante float do ELF, mais busca de BL/B imediato. Sem execução do APK.

Resultado: DoMethod mapeia0–5 para Init/Update/Draw/Resume/Suspend/Finish; slot19 é Update. Fórmula inteira de quantidade de passagens e correção+0,1f recuperadas, sem substituí-las por algoritmo convencional de timestep. Form+0x48 incrementa uma vez após o loop, não por chamada Update. Relatório28.

SEB: getters confirmam frame+0x18/máximo+0x1c; AutoAnimation é flag global. Frame manual é condicionado a AutoAnimation desligado e flag do framework. Caminho automático incrementa objetos registrados, deduplica por flag e pode zerar frame quando TotalFrames difere em pelo menos2. Reset limpa registro/quantidade ativa, não frames nem coleção. Draw continua mesmo se Push retornafalse. AnimationSystem.Update também mantém contador próprio no componente da entidade, com incremento configurável e chamada RestoreAnimation. Relatório29.

Validação:15 novos intervalos FDE (14SEB/chamadores,1dispatcher), cobertura sintática integral; fórmula confrontada com SDIV/CSINC/MSUB, tabela de salto com bytes do ELF. Sem alterações ao código executável do port, portanto não houve novo build/teste de comportamento. Confiança confirmed para operações descritas; seleção runtime e frequência permanecem desconhecidas.

Próximo: RestoreAnimation/componentes, ligação do sistema ECS ao evento1 e escritores dos flags/relógios. Goal ativo; ainda há avanço estático útil.

## 2026-10-25 — Transições de animação portadas

Pergunta: como interromper e restaurar animações por entidade? Método:19 FDEs delimitados, getters identificados no mapa IL2CPP, confronto de ADD/CSEL/AND/stores, revisão independente do módulo contra assembly.

Achado: Change/Restore usam humano ou monstro para direção; CutIn usa humano ou inimigo, com guarda de direção. CutIn armazena argumento explícito de retorno, sem pilha; Restore aplica agrupamento signed de quatro, zera frame e consome retorno ao fim. Ordem das falhas por componente ausente também recuperada. Serializadores dos componentes estreitam campos32 para byte/short; serializers do sistema são vazios, sem inferir save vazio.

Implementação: src/core/animation_transitions.hpp/.cpp, sem APIs de plataforma, assets, relógio ou alocação normal; inteiros32 com wrap definido, campos optional para presença. Testes cobrem troca, retorno substituído, no-op por sentinela inclusive sem Seb, direção/predicados distintos, mutações parciais, negativos/INT_MIN/overflow. Revisão independente não encontrou divergência nos três corpos sob getters estáveis; recomendação de teste adicional incorporada.

Validação: tools/build-desktop.cmd passou e CTest4/4, inclusive regressões de formatos. Log local build/research/animation-transitions-build.log. Nenhum teste dinâmico do APK, nenhuma equivalência temporal/visual alegada; Linux/3DS ainda não compilados. Relatório30. Próximo: produtores dos componentes e ordem de registro/execução ECS, para integrar o módulo sem inventar defaults ou scheduler. Goal permanece ativo.

## 2026-10-25 — Produtores e cadeia ECS

Pergunta: quem fornece os valores de animação e quando AnimationSystem.Update é despachado? Método:18 novos FDEs com cobertura sintática integral, leitura de parâmetros metadata, varreduras BL/B e conferência de interfaces/vtables.

Produtores: AddAnimation grava os dois argumentos após obter/alocar componente; AddSeb grava res/ID/frame/layers, inclusive array unitário[-1]. Human usa res1/ID36+dir/frame0, rate1/backup-1 e direção não travada. MapChip/Nature/Effect e Enemy adicionam animação sob condições explícitas; Enemy permite frame inicial vindo dos dados e ausência de direção quando dir=-1. Não foram adicionados defaults globais. Outros produtores Emitter/Projectile continuam a investigar; relatório31.

Integração estática: fábrica cria AnimationSystem no índice19; listas por interface preservam ordem relativa inicial. Init usa filtro de componentes2/10 e24 ResourceManagers. Evento1 do QuestForm chega a World.Update, SystemManager.Update e interface IUpdateSystem, com vtable comprovada de AnimationSystem. QuestForm tem loop interno de1/3 passagens com guards; NeedsUpdate da animação retorna true e não elimina as extras. Não equivale a um avanço por frame desenhado; relatório32.

Validação: limites/bytes dos18 exports conferidos, chamadas/argumentos dos produtores e seletores virtuais documentados. Nenhum C++ alterado nesta rodada, sem repetição desnecessária do build nem execução Android. Ainda há investigação estática útil: manutenção das listas por enable/disable, flags de Enemy e os produtores restantes. Goal ativo.

## 2026-10-25 — Ciclo ECS e produtores restantes

Pergunta: enable/disable/pause retiram sistemas de Update? Método:12 FDEs de lifecycle, confronto com manager/world já exportados e vtables BaseSystem/BasePauseSystem/AnimationSystem; mais2 FDEs Emitter/Projectile.

Resultado: setter enabled muda byte antes do hook somente em transição; manager notifica mesmo em pedido repetido. Não há remoção/reinserção direta das listas nesses métodos. AnimationSystem herda hooks vazios e NeedsUpdate retorna true; não implementar filtro genérico por enabled no dispatcher examinado. set_isPause apenas grava; PauseSystems faz fases de callbacks diferentes. RemoveAllSystems desabilita/notifica antes de limpar. Deserialize cria/adiciona sistemas e despacha slots Deserialize/Extra/OnDeserialized; preparação do caller continua pendente. Relatório33.

Produtores: Emitter adiciona Animation(1,-1) à partícula criada, sob filtros do emissor e hasBullet; Projectile.Fire adiciona ao argumento projectile somente se anim ligado, independente de rotate. Sete callers diretos conhecidos agora examinados; sem defaults globais ou prova de ausência de stores/indireções. Relatório34.

Validação:14 novos FDEs integralmente cobertos; assinaturas/slots/argumentos documentados; nenhum C++ alterado nem novo teste de runtime alegado. Próximo: pausa concreta, preparação de World.Deserialize e contrato de componente duplicado. Goal ativo, caminhos estáticos continuam disponíveis.
