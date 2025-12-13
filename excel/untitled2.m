% Caminho do CSV
csv_path = 'stability_timeseries.csv';

% Ler CSV com separador ';'
opts = detectImportOptions(csv_path, 'Delimiter', ';');
df = readtable(csv_path, opts);

% Coluna de tempo (normalmente é a primeira)
time_col = df.Properties.VariableNames{1};
tempo = df.(time_col);

% Procurar coluna de Erlangs
if any(strcmp(df.Properties.VariableNames, 'CarriedErlangs'))
    erlangs_col = 'CarriedErlangs';
else
    cols = df.Properties.VariableNames;
    idx = find(contains(lower(cols), 'erlang'), 1);

    if isempty(idx)
        error('Não encontrei nenhuma coluna que pareça conter Erlangs.');
    else
        erlangs_col = cols{idx};
    end
end

erlangs = df.(erlangs_col);

% Plot
figure;
plot(tempo, erlangs, 'LineWidth', 1.2);
xlabel(time_col);
ylabel(['Erlangs (' erlangs_col ')']);
title('Erlangs ao longo do tempo');
grid on;
axis padded;

% Guardar imagem
saveas(gcf, 'grafico_erlangs.png');
