# Marcações das tabelas e entrada do parser de texto

Data: 2026-10-25. Capítulo4/M3. Análise estática; nenhum texto ou asset proprietário adicionado ao Git.

## Inventário reproduzível

Estendido tools/inspect_presentation_table.py com inventário lexical anterior à tradução/substituição. Registra candidatos entre < e >, contagem e localizações por ordinal de registro/ID/fragmento; não interpreta tags, não remove desconhecidos e distingue candidatos numéricos de argumentos. Ordinais preservam registros com IDs duplicados.

| Tabela | Fragmentos | Candidatos encontrados |
| --- | ---: | --- |
| English Talk | 119 | br136; co=4a86e8:4; /co:4; argumentos0/1:1 cada |
| Japanese Talk | 119 | br182; argumentos0/1:1 cada |
| English EventMsg | 24 | br26; po=cm:23; argumento0:2 |
| Japanese EventMsg | 24 | br27; argumento0:2 |

As contagens são ocorrências, não número de registros. Evidência local com hash e localizações: build/research/presentation-markup/*.json. Esses resultados não limitam todas as tags possíveis: argumentos, tradução, tabelas adicionais e código podem introduzir outras.

Comando reproduzível para uma tabela:
```powershell
python tools/inspect_presentation_table.py talk data/extracted/members/06_xls/0017_English.lproj_Talk.txt --output build/research/presentation-markup/talk-full.json
```

## Transformações anteriores à leitura das tags

Em TextLayout._makeTextBlocks19200 (VA0x1bbd4c4), o trecho0x1bbd9a4–0x1bbda18 chama:

1. Language.TranslateText14007 com segundo argumento true.
2. Language.UnicodeToChanakya14048 (0x1b2ca38).
3. StringUtil.Replace14532, substituindo literal `<br>` por LF.

Os dois literais vêm dos GOT0x27323c0 e0x272fa98. Portanto a tradução do texto inteiro é uma dependência adicional à tradução dos argumentos implementada por host em88. Não presumir que a entrada das tabelas chegue inalterada ao parser.

## Tabela original de tags

TextLayout..cctor19191 (0x1bbc290) cria19 entradas e grava TAG no offset estático0x18. Literais resolvidos pelos metadados e descritores do ELF:

| Índice | Tag |
| ---: | --- |
| 0 | `<co=>` |
| 1 | `</co>` |
| 2 | `<si=>` |
| 3 | `</si>` |
| 4 | `<st=>` |
| 5 | `</st>` |
| 6 | `<bl=>` |
| 7 | `</bl>` |
| 8 | `<po=>` |
| 9 | `<pic=>` |
| 10 | `<bo=>` |
| 11 | `</bo>` |
| 12 | `<btn=>` |
| 13 | `<ff>` |
| 14 | `<kr=>` |
| 15 | `<em>` |
| 16 | `</em>` |
| 17 | `<ruby=>` |
| 18 | `</ruby>` |

As tags com = são modelos de prefixo; não significam que o parâmetro esteja vazio no conteúdo. `<br>` não está nesse array porque é convertido antes. `<0>`/`<1>` são candidatos a substituição anteriores ao parser; não foram classificados como comandos desse array.

O scanner em0x1bbdad4 procura < e o próximo >, verifica distância<=32 e percorre TAG em ordem. Usa String.CompareTo552 para igualdade e String.StartsWith566 com prefixo até = para as parametrizadas. Sem prova de comparação ordinal/cultura neutra, não implementar uma comparação diferente silenciosamente. Limites, ausência de fechamento e fallback literal exigem manter o fluxo original.

## Cor e posicionamento usados no conteúdo

A tabela de saltos uint16 em0x68dd0e resolve o índice0 em0x1bbeaac, índice1 em0x1bbec9c e índice8 em0x1bbee5c, base0x1bbe854 + entrada×4.

- co usa conversão numérica, separa RGB em três bytes e chama a fábrica de cor. /co restaura a cor do TextLayout+0x20; esse trecho não desempilha uma pilha de cores. Conversões decimal/hex e fallback de parâmetros ainda precisam de análise antes de portar.
- po procura l/c/r e aplica bits horizontais1/2/4 nessa ordem; procura t/m/b e aplica bits verticais0x10/20/40 nessa ordem. Se nenhuma letra da dimensão estiver presente, mantém a configuração anterior; letras posteriores prevalecem. Também zera o acumulador vertical local. cm seleciona2 e0x20, consistente com centro/meio; geometria final ainda não reconstruída.

Evidências: build/research/text-tags/M19191.asm e text-block-length/M19200.asm. VAs são do ELF ARM64 inventariado; exports Ghidra têm base adicional0x100000. Confiança **confirmed** para literais, chamadas, destinos e escritas citadas; fidelidade visual **unknown**.

## Validação e próximo passo

Sete testes Python passaram:
`python -m unittest discover -s tests -p test_presentation_table.py`.
Incluem nulos, contagem de strings, CR, IDs duplicados e candidatos desconhecidos. Não validam o parser visual. Sem mudanças C++, não repetidos os17 CTests de92.

Próximo: fechar o comportamento dos parâmetros e geração de blocos para br/co/po, junto das métricas de fonte. Não substituir o parser por remoção genérica de tags ou supor que desconhecidos desaparecem.
