# Configuração e filtros de tradução do save

Data:2026-10-25. Evidência estática do mesmo ELF ARM64 de56; endereços sem base Ghidra0x100000. Nenhum APK executado ou save acessado.

## Valor inicial não é valor permanente

Config..cctor13622 (`0x1bed0a0`,1.468 bytes) obtém seus campos estáticos e escreve zero com STUR WZR em+0x51 no site0x1bed33c. A escrita cobre bytes0x51–0x54, incluindo SAVEDATA_LOCALIZE+0x54, identificado em56. Assim, seu valor inicial é false nessa etapa. O contexto da base vem do GOT de Config e ponteiro de campos estáticos+0xb8; não é o mesmo offset em um objeto qualquer.

Config.Create13609 (`0x1beb228`,1.376 bytes) contém chamada a FieldInfo.SetValue8595 (`0x1d908a4`) no site0x1beb71c. A cadeia completa de seleção de campos e valores ainda precisa ser recuperada. A presença desse mecanismo impede concluir, só pelo inicializador, que o valor da flag nunca mude. Config.Init13611 (`0x1beb8dc`,4.640 bytes) foi exportado para seguir a configuração efetiva; export não equivale a interpretação de todos os seus efeitos.

## ReadTranslate: caminho com tabela disponível

StreamUtil.ReadTranslate14508 (`0x1c0d8d8`) primeiro consulta StreamUtil.TranslateEnable, campo estático+0x18. Se false, retorna a string recebida. StreamUtil.translateTable_ fica no campo estático+0x0. Quando a tabela é null, há caminho de inicialização por callback; sua origem e sincronismo ainda não foram esclarecidos. Não presumir que essa rotina carregue toda a tabela antes de qualquer tentativa.

No caminho com tabela disponível, filtros em0x1c0db1c–0x1c0dbac devolvem a entrada sem tradução quando:

- comprimento da string é maior que127 (campo de comprimento da string, não comprimento UTF-8);
- Regex.IsMatch com padrão `[^0-9]+` é false;
- String.StartsWith reconhece `http://`, `https://` ou `GPA.`.

Métodos de Regex/StartsWith/Replace foram associados pelos endereços do mapa IL2CPP. Literais foram resolvidos dos GOT para os bytes da metadata. O padrão não deve ser substituído por uma regra mais ampla de “texto numérico”: ele trata especificamente os caracteres0–9.

Para os candidatos restantes, a chave é construída nesta ordem por String.Replace:

1. CRLF → LF.
2. LF → sequência literal barra invertida+n.
3. TAB → sequência literal barra invertida+t.
4. Caractere0x08 → string vazia.

Em seguida consulta Dictionary.ContainsKey pela chave normalizada. Se não houver entrada, retorna a string **original**, não a chave normalizada. Se houver, obtém o valor e desfaz escapes de newline e tab, nessa ordem, retornando a tradução. Não há reversão para0x08, removido da chave. Valores null ou falhas intermediárias podem chegar aos helpers de erro; não foi implementada tolerância adicional.

As referências genéricas GOT0x273eca0/0x273fd58 resolvem para Dictionary`2.ContainsKey12533/get_Item12526, instanciação298. Os demais literais relevantes são documentados pelos exports do relatório56 e pela sequência acima. Essa normalização não é uma desserialização genérica de escapes: não inferir suporte a aspas, Unicode escapado ou barras invertidas arbitrárias.

## Limites e próximos passos

A tradução no save depende de duas guardas distintas: Config.SAVEDATA_LOCALIZE controla a chamada pelo leitor de strings; StreamUtil.TranslateEnable controla a operação interna. Confirmar uma delas isoladamente não encerra o comportamento. WriteTranslate, origem da tabela, valores efetivos das flags e restante de GetCode continuam pendentes.

Validação: novo export Config..cctor/1.468 bytes e Create/Init/6.016 bytes, todos com cobertura sintática integral. Leitura de ReadTranslate reaproveitou o export de56. Nenhum C++ alterado ou teste repetido. O codec completo de DelayEvent segue aguardando esses contratos, sem fixar flags ou substituir detecção por UTF-8 obrigatório.
