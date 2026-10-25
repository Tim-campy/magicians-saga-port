# Forma dos grupos e consumidor em NewGame

Data:2026-10-25. Evidência estática do ELF ARM64 identificado em51; VAs sem a base0x100000 adicionada nos exports Ghidra. Nenhuma execução do APK ou acesso a saves.

## MakeArray

O call site de InitCheckScriptData usa JTool.MakeArray13203, instanciação3905. A tabela de instanciações no ELF resolve seu único argumento para **data.BaseData[][]**, seguindo relocations dos ponteiros SZARRAY. O corpo compartilhado chamado está em0x12d0570,348 bytes.

O wrapper obtém o tipo por Type.GetTypeFromHandle2768, acessa assembly e nome completo, retira os dois caracteres finais via String.Substring615 e resolve o tipo correspondente. Esses slots de RuntimeType são27/get_Assembly e26/get_FullName. Encaminha tipo, dimensões e profundidade0 para JTool.MakeArrayRecursive13204 (`0x1b0572c`,420 bytes), e verifica o tipo do resultado antes de retornar. Lista de dimensões vazia retorna null; null chega ao helper de erro. Não equiparar todos os caminhos a uma alocação infalível.

MakeArrayRecursive chama Array.CreateInstance3252 (`0x1e87370`) com a dimensão atual. Havendo dimensão seguinte e tipo de elemento resolvido não nulo, percorre o comprimento do array e cria uma nova linha recursivamente para cada posição, gravada por Array.SetValue3246 (`0x1e83d10`). As consultas de comprimento são Array.get_Length3231 (`0x1e79f3c`); comparação de tipo com null usa Type.op_Equality2778 (`0x1e79554`).

Consequência para o caminho bem-sucedido com BaseData[][] e dimensões[5,0]: array externo com cinco posições, cada uma recebendo um BaseData[] vazio criado separadamente. Não são cinco referências null nem um único array compartilhado. InitCheckScriptData substitui as posições1–4 pelos resultados dos filtros descritos em52; posição0 conserva a linha vazia. Nos tipos auditados, GetTiming retorna0 e os filtros positivos não selecionam objetos.

## Consumidor em NewGame

Releitura do export anterior de AppData.NewGame34263: em0x11020b0 lê AppData+0x248 e percorre todas as linhas, começando em0. Para cada objeto, chama slot16; somente se o bit0 do retorno for1 chama slot15. Vtables dos22 tipos de51 resolvem esses slots como BaseData.CheckNotFind38060 (`0x116c7c0`) e BaseData.CheckTerm38059 (`0x116c7b8`), respectivamente.

Ambos os FDEs têm8 bytes: `e0031f2ac0035fd6`, correspondentes a retorno zero seguido de RET. Portanto não introduzir efeitos de desbloqueio ou limpeza do World a partir dos nomes desses métodos. Para os grupos construídos conforme52, as linhas vazias já impedem essas chamadas. São conclusões sobre este caminho e versão, não sobre todo sistema de eventos.

## Busca delimitada e scripts separados

Uma busca nos FDEs mapeados de AppData por instruções LDR/STR unsigned-immediate de32/64 bits com offsets0x238/0x240/0x248/0x2e8 produziu `build/research/checkscript-field-candidates.json`. Os candidatos abrangem construtor, NewGame, InitCheckScriptData e inicializador estático. Os acessos+0x238/+0x240 em NewGame são ao vtable do objeto, não aos campos de AppData: falso positivo removido por leitura do contexto. As escritas no inicializador estático também não demonstram escrita nos campos de uma instância. A busca não cobre todos os modos de endereçamento, outros tipos ou acessos indiretos e não prova ausência de consumidores adicionais.

Os métodos ExecuteScript34377/34378/34379, GetScriptText34380, GetDelayScript34383 e SerializeScripts34410/DeserializeScripts34411 foram exportados para a próxima investigação. A leitura inicial de ExecuteScript34377 (`0x110c8ec`) já mostra execução sequencial por outro overload e ramo especial quando o primeiro inteiro da instrução é10, com criação de objeto e inserção na coleção AppData+0x200. O significado desse ramo, argumentos, operações e persistência ainda precisa ser resolvido. Não confundir os grupos vazios de verificação com inexistência de scripts.

## Validação e próximos passos

`script-checks/coverage.tsv`: nove FDEs,5.192 bytes, incluindo uma reexportação de InitCheckScriptData. `makearray-helpers/coverage.tsv`: três FDEs,948 bytes. Cobertura sintática integral; alguns exports são preparação de pesquisa, não contratos já compreendidos. Os dois retornos constantes adicionais foram conferidos diretamente nos bytes e intervalos FDE. Nenhum C++ alterado ou teste de runtime alegado.

Próximo: semântica dos opcodes/argumentos de ExecuteScript, objeto de execução adiada e formatos SerializeScripts/DeserializeScripts. População de Data e preparação global do World permanecem abertas. Ainda há investigação estática útil.
