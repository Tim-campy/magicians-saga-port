# Eventos adiados no bloco2 do save

Data:2026-10-25. Evidência estática do mesmo ELF ARM64 de54. Endereços são VAs ELF; exports Ghidra somam0x100000. Nenhum save real acessado ou APK executado.

## Distinção do codec anterior

Busca de instruções BL/B imediatas alinhadas nos segmentos executáveis não encontrou destinos AppData.SerializeScripts34410 ou DeserializeScripts34411. Resultado local `build/research/script-payload-callers.json`: array vazio. Isso não exclui chamadas indiretas/reflexivas ou outros módulos. O codec de54 permanece sustentado pelos corpos dos métodos, mas sua participação no save não está comprovada e não deve ser presumida.

A mesma busca encontrou DelayEvent.Serialize35500 chamado em PackSave34314 no site0x1107500, e DelayEvent.Deserialize35501 chamado em LoadGame34324 no site0x11087d4. Evidência local `delay-event-callers.json`; contexto relido dos exports `save-callers/`.

## Ligação concreta com o save

PackSave compara seletor com2 em0x11073bc e entra no ramo0x1107440. Lê coleção AppData+0x200, grava Size como int32 BE, e percorre seus objetos em ordem, conferindo o tipo e chamando DelayEvent.Serialize. O laço reconsulta Size; não há snapshot local. O bloco13 do World permanece um ramo diferente.

LoadGame primeiro cria novo Vector e substitui AppData+0x200 em0x1108738. Obtém elemento2 do array em SecureProperty+0x10 (objeto SecureProperty em AppData+0x198), cria ByteArrayInputStream, lê quantidade int32 e, para cada entrada, constrói DelayEvent vazio, desserializa e só então adiciona à coleção. Quantidade<1 produz coleção vazia. Falha durante uma entrada pode deixar as anteriores já adicionadas; não há transação local de restauração da coleção antiga.

Essa sequência ocorre depois de World.Deserialize no corpo previamente examinado, antes da saída de LoadGame. Confirma o vínculo do bloco2 à execução adiada; ainda não estabelece o envelope externo SecureProperty/RecordStore.

## Layout com nomes comprovados

Metadata tipo4249 contém sete campos. A tabela nativa de offsets, ponteiro em0x277ece8+8*4249, resolve para0x65b4b8 com valores[16,20,24,28,32,40,48]. Assim, a associação não depende apenas da ordem aparente das instruções:

| Offset | Campo |
|---|---|
|0x10|delayId_|
|0x14|line_|
|0x18|wait_|
|0x1c|hour_|
|0x20|talkChara_|
|0x28|args_|
|0x30|argi_|

Os nomes não provam as unidades de wait_/hour_, nem o significado de sentinelas. O construtor vazio35498 (`0x11c94a0`,104 bytes) grava os cinco inteiros[-1,0,0,-1,-1] e args_ como string vazia. Constante SIMD lida em0x625fa0; referência de literal GOT0x272ee88 resolve para literal de comprimento0. O corpo não escreve explicitamente argi_; não confundir isso com escrita de null pelo construtor.

Construtor35499 (`0x11c9508`,220 bytes) grava os cinco argumentos inteiros, conserva a referência args_ e faz cópia de argi_ quando fornecido; null mantém o campo explicitamente zerado, conforme54. A identidade dos argumentos inteiros vem da associação dos stores aos offsets acima.

## Formato de DelayEvent

Serialize35500 (`0x11c95e4`,412 bytes) grava cinco inteiros BE na ordem da tabela, depois args_ via StreamUtil.WriteString14496 (`0x1c0f834`), substituindo null por string vazia. Em seguida grava comprimento de argi_ como int32 BE e seus elementos int32 BE. Para argi_ null grava comprimento0.

Deserialize35501 (`0x11c9780`,332 bytes) lê os cinco inteiros e os grava imediatamente, lê args_ via StreamUtil.ReadString14463 (`0x1c0d810`), lê comprimento de argi_, aloca array e preenche. Comprimento0 produz array vazio; portanto null/vazio não são preservados pelo round trip. Comprimento negativo não é sentinela: chega à alocação inválida. Os campos podem ficar parcialmente alterados se leitura posterior falhar.

Formato da string, seus limites, encoding e comportamento de null/truncamento ainda não foram recuperados. Por isso nenhum codec completo de DelayEvent foi implementado nesta rodada. Não substituir WriteString por UTF-8 com prefixo presumido.

## Validação e próximos passos

`delay-event/coverage.tsv`: quatro FDEs,1.068 bytes, cobertura sintática integral, incluindo reexportação do construtor já visto. Chamadas de PackSave/LoadGame conferidas em contexto; constantes, literal e offsets lidos do ELF/metadata. Nenhum C++ alterado e nenhum teste repetido; resultado anterior8/8 permanece a última validação do código.

Próximo recorte: ReadString/WriteString para concluir esse payload e consumidor que decrementa/avalia wait_/hour_/line_. Há trabalho estático disponível; o objetivo permanece ativo.
