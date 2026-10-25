# Produtor do catálogo de falas e fronteira de tradução

Data: 2026-10-25. Evidência estática ARM64; VAs sem base adicional Ghidra0x100000. Nenhuma execução do APK.

## Ligação confirmada durante o boot

BootForm.Load35730 preenche o campo estático AppData.talks_ em0x11d4070. A cadeia recuperada é:

`Data.get_talkData38347 → Select<TalkData,string> → ToArray<string> → AppData.talks_`

O descritor GOT0x2733d58 resolve methodSpec57568, definição46536/Select e instância978 com argumentos TalkData/string. O descritor0x272f3a0 resolve ToArray, definição46556 e instância89/string. O delegate de Select vem de GOT0x2733d80: método35739, `<>c.<Load>b__40_2`, VA0x11d4f7c.

Esse delegate verifica o registro, lê TalkData.text em+0x28 e chama String.Join586, VA0x1cee558, com separador TAB confirmado pelo literal0x272feb8. Assim, para cada registro, o array contado de textos identificado em85 é unido por TAB; a posição correspondente no array resultante é usada posteriormente por MakeTalkForm36175. Não há chamada a TranslateText nesse delegate. A tradução dos argumentos no helper Replace é uma etapa posterior e distinta.

Registro null falha no delegate; não gera automaticamente uma fala vazia para lacunas. Nos arquivos auditados, os46 IDs consecutivos evitam essa lacuna, mas isso não altera o contrato de erro para outros dados.

## Desvio fora do intervalo do método

O trecho0x11d4000 desvia para0x25fe210 antes de construir o delegate. Os bytes do ELF original confirmam o desvio; não é uma instrução presumida a partir do pseudocódigo. O destino executa `ldr x9,[x9,#0xd68]` e retorna para0x11d4004. Esse pequeno bloco foi exportado separadamente para fechar o fluxo fora do intervalo FDE principal.

A varredura por store imediato+0x378 também encontrou AppData..ctor34232, mas trata-se de objeto de instância e não comprova escrita no campo estático. A identificação do produtor depende do caminho via classe+0xb8, dos descritores genéricos e do delegate, não apenas da coincidência numérica de offset.

Artefatos locais: `talk-store-candidates.json`, `talk-writers/M35730.asm`, `talk-delegate/M35739.asm` e `talk-delegate/TalkDelegateThunk.asm`. A cobertura sintática de um intervalo isolado não comprova todos os seus caminhos transitivos.

## O que TranslateText ainda exige

Language.TranslateText14007, VA0x1b1e6f0, foi examinado separadamente. Null retorna null; string vazia retorna a própria string. Para texto não vazio, consulta GetTranslateCache14035 e retorna imediatamente quando encontra resultado não null.

Sem esse resultado, chama CallTranslateText14008 com o booleano recebido, possui guardas adicionais para fallback a TranslateBuiltInMultiLangText14038 e TranslateConsoleText14039 e termina chamando SetTranslateCache14036. Não equivale a um simples acesso único a dicionário.

CallTranslateText em0x1b1ebf0 contém tratamento de caracteres especiais, substituições, regex, trim, conversão de caixa e chamadas a _translateText14009. Este último também pode refazer a busca após tratamento por regex. Os corpos foram exportados em `talk-origin`, `talk-delegate` e `translation-lookup`, mas seus flags, literais e ordem completa de fallback ainda não estão fechados. Nomes dos métodos não são prova suficiente para substituir essas rotinas por funções genéricas de localização.

Portanto a montagem de talks_ está resolvida neste recorte, mas a tradução, a origem do idioma efetivo e a limpeza/trim descritos em85 permanecem pendentes antes do modelo nativo de apresentação completo.

## Cruzamento com os dados e próximo passo

Os46 registros de Talk.txt de cada idioma contêm119 fragmentos;40 registros têm mais de um fragmento e nenhum tem array vazio. A união por TAB é necessária para conservar as fronteiras usadas posteriormente pelo formulário. Não confundir os119 fragmentos com119 páginas visualmente verificadas.

Contagens calculadas a partir dos JSONs da auditoria85, sem publicar texto do jogo. Nenhum C++ ou parser foi alterado; não foram repetidos testes. Últimos resultados permanecem13/13 CTest Windows (`84`) e cinco testes do parser (`85`).

Próximo: fechar os caminhos de tradução realmente configurados pelo boot e o trim usado na divisão dos textos; então implementar preparação nativa preservando identidade, segmentos e ordem de substituição. Ainda há avanço estático útil.
