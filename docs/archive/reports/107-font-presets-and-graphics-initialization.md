# Presets de fonte e inicialização de Graphics

Data: 2026-10-25. Capítulo 4/M3. Investigação estática; nenhuma execução do APK.

## Tabelas iniciais comprovadas

Font..cctor17820 (0x1b7c070) aloca dois arrays de três elementos. Chama RuntimeHelpers.InitializeArray8401 (0x1d891c4) em 0x1b7c19c e 0x1b7c1c4; guarda os resultados nos campos estáticos HEIGHT+0 e WIDTH+8 em 0x1b7c1ac e 0x1b7c1d4.

| Campo | GOT do descritor | FieldRef | Campo nos metadados | Offset dos dados nos metadados | Três int32 LE |
| --- | --- | --- | --- | --- | --- |
| HEIGHT | 0x273d7e8 | 181 | 11518 | 0x448428 | 12, 24, 30 |
| WIDTH | 0x273d7f0 | 188 | 11525 | 0x448770 | 6, 12, 15 |

Os dois FieldRefs apontam ao tipo nativo 8812, resolvido para a definição 2349, `<PrivateImplementationDetails>`, campos relativos 26 e 33. Os registros field_defaults fornecem os offsets acima. Os SHA-256 dos 12 bytes são, respectivamente:

- `33cba97773573cf829ff1c612a030157fc39d759d92e4527aafc46a833623bc5`
- `3f99a41594675e89f2e805f9ca9c1a6c8fb6ec591a90f22fb77b7e7904e8596b`

Esses hashes coincidem com os nomes dos campos no APK. O cruzamento comprova os valores iniciais, não a imutabilidade das tabelas durante toda a execução. O módulo text_font continua recebendo a tabela explicitamente até resolver seus produtores posteriores e integração ao boot.

## Nomes originais e campos distintos

A tabela nativa de offsets confirma `size_` em +0x10, `usedSize_` em +0x1c e `guiFontSize_` em +0x2c. No módulo106, correspondem a requested_size, size e adjusted_size. Referências anteriores a “size_+0x1c” descreviam o tamanho efetivo corretamente, mas usavam o identificador original incorreto: nesse offset o nome é **usedSize_**. Preservar essa distinção ao consultar métodos e notas.

## Produtor de useFontTexture_

Graphics.Init17848 (0x1b5c4e8) cria fontes se os campos +0x1c8 e +0x1d0 estão nulos. Chama Font.Init sobre a primeira em 0x1b5c8ec. No trecho 0x1b5c978..0x1b5c988, lê o byte estático de Graphics+0x59 e grava no campo de instância Font+0x48 dessa primeira fonte.

O GOT0x272f868 resolve para Graphics; seu campo estático +0x59 é `<DefaultUseFontTexture>k__BackingField` (índice9987). Portanto há um produtor concreto de useFontTexture_ além das cópias já documentadas. Isso não prova que o valor seja true nem que TextLayout use essa mesma instância: a origem de DefaultUseFontTexture e sua propagação às falas permanecem abertas.

Canvas.Init17583 (0x1b4ae78), no recorte 0x1b4c030..0x1b4c044, também cria uma Font e guarda em seu campo+0x20. Não foi observada alteração das tabelas de presets nesses recortes de criação.

## Escala global: escrita inline

Language.Init13986 contém store de 100 em Font.requestFontScale_+0x44 em 0x1b14c28 e zero em requestFontOffset_+0x48 em 0x1b14c5c, sob guarda anterior do campo de configuração +0x60. LoadLanguagePack14015 também tem store de 100 em 0x1b23ffc; seu caminho completo e a interpretação da guarda continuam pendentes. Esses stores inline explicam por que ausência de chamadas diretas a SetScale não implica ausência de produtores. Não adotar 100 como valor efetivo universal.

## Evidências, cobertura e próximo passo

Locais: `build/research/font-presets/initial-tables.json`, `F1b4ae78.asm`, `F1b5c4e8.asm` e coverage.tsv; Font..cctor em `font-selection-default/M17820.asm`; Language em `language-font-origin/`. ELF SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Os dois novos exports cobrem sintaticamente 7.816 bytes, não equivalência de execução.

A busca auxiliar `font-type-reference-scan.json` identifica candidatos por ADRP e LDR nas sete instruções seguintes. Não rastreia redefinição dos registradores, aliases ou todas as formas de endereçamento: contém falsos positivos e não sustenta ausência global de writers. Apenas os trechos explicitamente conferidos acima são conclusões.

Confiança **confirmed** para arrays iniciais, offsets e stores descritos; backend efetivo das falas **unknown**. Sem mudança C++ ou repetição de testes; último build20/20 em106. Próximo: origem de DefaultUseFontTexture e propagação Graphics→TextLayout, mantendo a configuração real como pré-requisito da integração visual.
