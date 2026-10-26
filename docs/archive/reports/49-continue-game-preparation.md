# Preparação no caminho Continuar

Data: 2026-10-25. APK SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`; ELF ARM64 SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. VAs ELF, exports com base Ghidra adicional0x100000. Nenhum APK executado.

## Caller direto recuperado

Busca de instruções BL/B alinhadas nos segmentos executáveis encontrou um caller direto de AppData.LoadGame34324: **TitleForm.ContinueGame36243**, site0x1218ae4, dentro do FDE0x1218a14–0x1218d28. Resultado local `build/research/loadgame-direct-callers.json`. A busca não cobre indireções/reflexão nem estabelece unicidade de todos os caminhos possíveis.

No ramo em que AppData.FlagCheck(4) retorna true, ContinueGame atualiza soft labels e faz esta sequência:

1. AppData.NewGame34263 em0x1218ad0, com segundo registrador de argumento explícito zero.
2. AppData.LoadGame34324 em0x1218ae4, slot0.
3. AppData.InitWorld34262 em0x1218af4.
4. Constrói SelectForm35963, guarda no campo AppData+0xa0 e chama FormManagerBase.ChangeCurrentForm.

No outro ramo prepara um diálogo. Os campos/finalidade da flag4 e os caminhos excepcionais não foram recuperados por completo. Não chamar LoadGame isoladamente no port presumindo que ele execute todas as etapas de preparação e transição de tela.

## NewGame antes de carregar

AppData.NewGame34263 (`0x1101d58`,1420 bytes) chama SaveRecorder.SaveSystem18925 antes de criar um novo SecureProperty em AppData+0x198. Depois chama InitSystemSaveData34261 e NewData34264. Também recria coleções e inicializa campos de dados; o corpo não foi convertido em implementação portátil nesta rodada.

O fato de o caller passar zero no segundo registrador não foi interpretado como um controle funcional: o corpo examinado não preserva esse argumento para condicionar a chamada inicial SaveSystem. Não inventar opção de “preparação sem escrita”. Nenhuma dessas funções foi executada pelo agente; os possíveis efeitos em storage são somente evidência estática.

NewData34264 (`0x11022e4`,280 bytes) chama **GetDataList34321** em0x1102318. Percorre os descritores, converte o primeiro elemento de cada um para BaseData[] (tipo resolvido em48), e para cada objeto não nulo chama slot18. A vtable de BaseData4503 identifica slot18 como **BaseData.NewGame38069**. Esse despacho virtual pode executar overrides; não substituir por zerar todos os campos ou simplesmente realocar arrays.

Ao terminar, NewData faz tail call para **InitCheckScriptData34265**,0x1102b4c. Esse método foi exportado (1060 bytes); as chamadas diretas identificadas incluem GetDataList e Vector..ctor. Sua operação completa e os campos alterados ainda não foram reconstruídos. O nome não é evidência de uma limpeza ECS.

## O que isso fecha e o que permanece aberto

A preparação anterior a LoadGame foi localizada no fluxo de Continuar, e inclui reinicialização dos objetos BaseData por métodos virtuais. InitWorld acontece depois da carga, coerente com o teste de World+0x38 discutido em32/36. Ainda não se identificou aqui uma chamada explícita que limpe o registro ECS antes de EntityManager.Deserialize.

As possibilidades restantes precisam ser distinguidas por evidência: mundo ainda vazio ao entrar no título, limpeza numa transição anterior, ou efeito de um override BaseData.NewGame. Não escolher uma delas por convenção. O próximo alvo é GetDataList e os tipos concretos cujos NewGame são despachados, junto dos callers de limpeza do World.

## Validação

Exports `build/research/continue-game.tsv`, `newgame-preload.tsv`, `newdata-preload.tsv` e `newdata-tail.tsv`, todos com coverage.tsv de cobertura sintática integral. Chamadas e ordem do ramo Continuar, métodos do mapa IL2CPP e slot18 conferidos. A exportação integral de NewGame/InitCheckScriptData não equivale à compreensão de todos os efeitos.

Nenhum C++ alterado ou teste repetido, nenhum save aberto/escrito e nenhuma execução de NewGame. Goal ativo; investigação estática continua útil.
