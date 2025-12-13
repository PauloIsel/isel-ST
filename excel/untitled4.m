function B = erlangB(A, C)
% Calcula a probabilidade de bloqueio Erlang B
% A: tráfego em Erlangs
% C: número de linhas/canais
    B = zeros(size(A));
    for k = 1:length(A)
        a = A(k);
        b = 1;
        for i = 1:C
            b = (a * b) / (i + a * b);
        end
        B(k) = b;
    end
end

% Caminho do CSV gerado pela simulação
csv_path = 'stability_timeseries.csv';

% Lê a tabela
opts = detectImportOptions(csv_path, 'Delimiter', ';');
df = readtable(csv_path, opts);

% Coluna de tempo
tempo = df.tempo;

% Coluna de Erlangs
if any(strcmp(df.Properties.VariableNames, 'CarriedErlangs'))
    erlangs = df.CarriedErlangs;
else
    idx = find(contains(lower(df.Properties.VariableNames), 'erlang'), 1);
    erlangs = df{:, idx};
end

% Número de canais (exemplo: 7 operadores)
C = 7;

% Calcula Erlang B para cada instante
B = erlangB(erlangs, C);

% Plot
figure;
plot(tempo/3600, B, 'LineWidth', 1.5); % tempo em horas
xlabel('Tempo [h]');
ylabel('Probabilidade de Bloqueio B');
title(['Erlang B ao longo do tempo (C = ' num2str(C) ')']);
grid on;
axis padded;

saveas(gcf, 'grafico_erlangB.png')
