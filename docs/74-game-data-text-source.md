# Origem textual das tabelas de gameplay

Data: 2026-10-25. Análise estática ARM64. Endereços ELF sem base adicional Ghidra0x100000. Nenhum jogo executado.

## Do contêiner às sequências

Data.Load38352 em0x1170508 constrói JarInflater com o argumento recebido e encaminha à sobrecarga38353. Esta monta21 nomes de tabelas, chama StringUtil.LoadStringArray(jar,nome,true) para cada nome, guarda os resultados num array e entrega a Data.Load38354. Não há confirmação nesta etapa sobre qual contêiner o caller fornece nem sobre a seleção efetiva de idioma.

O nome usado na posição de eventos é Event.txt: GOT0x2731d30 resolve uma string literal com esse conteúdo. A posição5 do array de nomes corresponde ao campo Data+0x38 e à instanciação LoadData<EventData> resolvida em73. Outros nomes amostrados são Cloth.txt e Voice.txt. Portanto não assumir que o nome Event.txt é apenas uma convenção deduzida dos arquivos extraídos.

## Leitura das linhas

StringUtil.LoadStringArray14549 (0x1c19ce4) chama JarInflater.GetInputStream13965 e depois a sobrecarga14551 (0x1c19dc0). O stream é fechado pelo caminho de descarte. Ausência de stream retorna null; tratamentos de exceção existem e não devem ser substituídos automaticamente por um array vazio.

A sobrecarga14551 lê bytes via StreamUtil.Read14433, converte com JString.New13436 e chama StringUtil.Split14535 com delimitador LF e flags(0,1,0) para esta chamada de Data.Load. Delimitador confirmado pela string literal do GOT0x272fa98.

No Split, essas flags desativam o ramo de trim e o tratamento de separadores escapados; ativam exclusão de segmentos de comprimento zero. Linhas contendo apenas espaços não são vazias por esse critério. Há um ramo adicional de normalização quando o separador é LF e uma configuração é verdadeira; a configuração e os literais dessa substituição ainda precisam de resolução antes de afirmar tratamento completo de CRLF. Não portar com splitlines/strip genéricos sem preservar essas diferenças.

JString.New verifica prefixo EF BB BF e, nesse caso, passa os bytes após os três primeiros para o decoder. O caminho sem BOM usa o mesmo provedor de encoding; sua identidade precisa ser confirmada antes de fechar o codec e a política para bytes inválidos. Não inferir Shift_JIS apenas porque há conteúdo japonês.

## Construção dos registros

Metadata identifica o delegate genérico como método38361 <LoadData>b__74_0, do tipo <>c__74`1. O corpo compartilhado LoadData realiza transformação/materialização antes de AdjustData por IDs, mas o corpo concreto do delegate ainda não foi localizado. Falta confirmar chamada a EventData.Load e política para linhas inválidas/null antes de implementar o loader integrado.

Exports novos em build/research/data-load-entry, data-text-reader, string-lines e string-lines-helpers, com cobertura/hash. Nenhum C++ alterado nem teste repetido; último build10/10 permanece70. O ciclo anterior foi progresso ao resolver indexação; este ligou a entrada Event.txt ao leitor e esclareceu filtros de linhas. Próximo: encoding/normalização e delegate de construção, mantendo a conclusão sobre1133 condicionada à cadeia completa.
