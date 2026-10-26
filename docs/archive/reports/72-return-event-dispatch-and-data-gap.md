# Evento de retorno: executor e lacuna nos dados

Data: 2026-10-25. Investigação estática do APK inventariado. VAs ELF sem base Ghidra0x100000; nenhum jogo executado.

## Executor confirmado

AppData.ExecEvent34374 encaminha para sobrecarga34376 em0x110c668, exportada em build/research/return-event. Essa rotina retorna null quando índice<0 ou índice>=Data.eventData.Length. Não procura por um campo ID dentro de uma coleção: usa diretamente o índice do array.

Para índice válido, obtém contexto padrão quando necessário e incrementa EventData.execNum_ (+0x20) em32 bits. Só depois verifica execsScript_ (+0x40): se não nulo e não vazio, encaminha para0x110c8ec. Caso contrário retorna null, podendo produzir diagnóstico conforme Config. Portanto evento válido sem script ainda altera contador; evento fora do array não o altera. Não há checagem direta de termsScript_ neste corpo.

Data.get_eventData38335 (0x116ff9c), exportado em event-data, retorna campo+0x38 do objeto Data estático. Sua extensão não é dedutível apenas da chamada com1133.

## Arquivos extraídos

Auditoria local em build/research/event-table-audit.json:

- English.lproj_Event.txt e Japanese.lproj_Event.txt são idênticos por SHA256 b625c2892879394ee150563771a09230b2176e29b7c670198fd7cb43bb5ca9fa.
- Cada arquivo tem228 linhas, com primeiro campo inteiro entre0 e228.
- Nenhum desses registros tem primeiro campo1133.

Esses fatos não bastam para provar que Data.eventData.Length<=1133: o loader pode dimensionar, preencher ou combinar dados de outra forma. Portanto o evento1133 chamado por TownSystem não foi classificado como penalidade, mensagem nem no-op definitivo. Se o array efetivo tiver comprimento<=1133, a guarda comprovada fará a chamada retornar null sem executar script.

## Leitura de EventData

EventData.Load38314 (0x116fa4c), exportado em event-loading, lê o primeiro inteiro para id_+0x18, uma string para name_+0x28, outro inteiro para timing_+0x30 e dois blocos para termsScript_+0x38 e execsScript_+0x40. A correspondência completa dos helpers de tokenização e scripts ainda precisa de auditoria. Metadata confirma os nomes dos campos, incluindo execNum_. Não confundir esse tipo data.EventData com o tipo homônimo de infraestrutura presente na metadata.

Data.Load38353/38354 também foram exportados em event-loading para rastrear construção/população dos arrays. A análise dessa cadeia permanece pendente; não substituir o loader por um array de228 posições com base na contagem de linhas.

## Estado

Ciclo anterior foi progresso ao confirmar handler do retorno. Neste ciclo foram obtidos contrato de despacho, localização do array, esquema inicial da linha e discrepância concreta entre índice1133 e dados extraídos. Nenhuma alteração C++ ou repetição de testes; último build10/10 permanece o de70. Próximo: resolver dimensionamento/população de Data.Load e origem efetiva da tabela, aproveitando essa investigação para os demais dados de gameplay.
