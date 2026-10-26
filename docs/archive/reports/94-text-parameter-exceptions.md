# Parâmetros numéricos e exceções locais do parser

Data: 2026-10-25. Capítulo4/M3. Análise estática ARM64, sem execução do APK.

## Pergunta

O parser de93 chama ParseInt decimal antes da tentativa hexadecimal. Como parâmetros de cor alfanuméricos chegam à segunda tentativa sem interromper o fluxo? Resolver somente os branches normais seria insuficiente.

## Helpers e estilos

- JInteger.ParseInt13380 (0x1b114f4) encaminha para Int32.Parse1984 (0x1e5fc68). Este fornece estilo0x7 e NumberFormatInfo obtido por chamada ao runtime.
- JInteger.ParseInt13381 (0x1b11578) só distingue radix16: nesse caso encaminha para Int32.Parse1985 (0x1e5fd28) com estilo0x200. Outros valores usam o mesmo caminho decimal.
- Nenhum desses wrappers captura exceções. Não implementar ParseInt globalmente como “retorne0 se inválido”. O host de parsing dos formulários de89 continua explícito.

Evidência: build/research/text-integer/M13380.asm, M13381.asm e text-integer-impl/F1e5fc68.asm, F1e5fd28.asm. VAs excluem a base Ghidra0x100000.

## Rotas de exceção comprovadas

FDE de _makeTextBlocks19200: início0x1bbd4c4, tamanho10344. LSDA0x935e34 possui255 intervalos de chamadas. O decoder local observa LPStart omitido, type encoding0x9c e call-site encoding ULEB128.

| Chamada/trecho protegido | Landing pad | Efeito confirmado por instruções |
| --- | --- | --- |
| Decimal0x1bbe418, intervalo [0x1bbe414,0x1bbe41c) | 0x1bbf300 | Após captura compatível, zera w27 e retorna a0x1bbe420, tentativa hexadecimal. |
| Hexadecimal0x1bbe42c, intervalo [0x1bbe420,0x1bbe430) | 0x1bbf2bc | Após captura compatível, zera w25 e retorna a0x1bbe434, seleção do tratamento da tag. |
| Primeiro canal RGB0x1bbe498 | 0x1bbf930 | Segue rota externa; não é o handler que zera localmente w27/w25. |

Os dois handlers locais verificam seletor1, iniciam a captura e verificam o objeto contra System.Exception/type348. Descritor GOT0x272f548 → native type11355 → type348. Caso incompatível, seguem o lançamento/propagação; o comportamento completo do handler externo permanece pendente.

Assim, a cor4a86e8 pode falhar no decimal, receber w27=0 e prosseguir para hexadecimal. Não inferir que todas as falhas do parser recebem o mesmo tratamento.

## Seleção do parâmetro de cor

Depois das duas tentativas, para tags co/bo (índices0/10), se o resultado hexadecimal for0, o parser procura vírgula. Sem vírgula mantém0; com vírgula divide os componentes, interpreta em decimal e combina os valores com shifts16/8 e OR. Os canais finais vêm dos três bytes baixos. Se hexadecimal não for0, não usa esse fallback.

O caminho de componentes RGB não possui as mesmas capturas locais das duas tentativas iniciais. Antes de portar todos os casos malformados, resolver o handler externo e a semântica de conversão/cultura do runtime. A leitura estática explica o caminho normal da cor existente, mas não valida desenho ou pixels.

## Ferramenta e validação

Novo tools/inspect_exception_callsites.py exporta intervalos e endereços dos landing pads com SHA-256 do ELF. Reutiliza pyelftools já instalado em .tools/python-libs; não instala dependências e não executa o alvo. Não interpreta catch types ou chains de ações automaticamente. Esses vínculos foram investigados separadamente no assembly.

```powershell
python tools/inspect_exception_callsites.py build/research/il2cpp/lib/arm64-v8a/libil2cpp.so --function 0x1bbd4c4 --output build/research/text-integer-exceptions.json
python -m unittest discover -s tests -p test_exception_callsites.py
```

Resultado:255 intervalos exportados; dois testes Python passaram, cobrindo limites/truncamento, encodings não suportados, overflow de ULEB e ausência de landing pad. Confiança **confirmed** nos intervalos, encaminhamentos e handlers citados; cultura completa e handler externo **unknown**. Sem mudança C++ ou repetição dos17 CTests.

Próximo: fechar a recuperação externa do parser e conversões usadas pelos parâmetros antes de ligar br/co/po aos blocos reais. Ainda há avanço estático útil.
