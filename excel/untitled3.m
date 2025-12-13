%% --- Leitura do ficheiro ---
csv_path = 'stability_timeseries.csv';

opts = detectImportOptions(csv_path, 'Delimiter', ';');
df = readtable(csv_path, opts);

% Nome da coluna do tempo (assumo 1ª)
time_col = df.Properties.VariableNames{1};
tempo = df.(time_col);

% Identificar coluna de Erlangs (pode ajustar se necessário)
if any(strcmp(df.Properties.VariableNames, 'CarriedErlangs'))
    vcol = 'CarriedErlangs';
else
    cols = df.Properties.VariableNames;
    idx = find(contains(lower(cols), 'erlang'), 1);
    vcol = cols{idx};
end

valor = df.(vcol);


%% --- Cálculo da estabilidade com janela móvel ---
janela = 50;                 % número de amostras por janela (ajusta se necessário)
sd = movstd(valor, janela);  % desvio padrão local

limiar = 0.01 * max(valor);  % 1% do valor máximo (ajusta conforme os teus dados)
estavel = sd < limiar;       % zonas com baixa variabilidade

%% --- Encontrar intervalos contínuos estáveis ---
% queremos apenas intervalos longos de estabilidade
min_dur = 100;  % mínimo de pontos seguidos para considerar estável
estavel_bin = bwlabel(estavel);  % identifica blocos ligados

inicio_estavel = [];
fim_estavel = [];

for i = 1:max(estavel_bin)
    idx = find(estavel_bin == i);
    if numel(idx) >= min_dur
        inicio_estavel = [inicio_estavel; idx(1)];
        fim_estavel = [fim_estavel; idx(end)];
    end
end


%% --- Plot final ---
figure;

plot(tempo, valor, 'b', 'LineWidth', 1.2); hold on;
xlabel(time_col);
ylabel(vcol);
title('Deteção automática de estado estável');

% pintar zonas estáveis
for k = 1:length(inicio_estavel)
    xpatch = tempo(inicio_estavel(k):fim_estavel(k));
    ypatch = valor(inicio_estavel(k):fim_estavel(k));
    patch([xpatch; flipud(xpatch)], ...
          [ypatch; flipud(ypatch)*0], ...
          [0.6 1 0.6], ...
          'FaceAlpha', 0.25, 'EdgeColor', 'none');
end

legend('Série', 'Zonas Estáveis');
grid on;
axis padded;

%% --- Mostrar tempos de estabilidade ---
disp('--- Zonas de Estabilidade Detectadas ---');
for k = 1:length(inicio_estavel)
    fprintf("Intervalo %d: de t = %.2f até t = %.2f\n", ...
        k, tempo(inicio_estavel(k)), tempo(fim_estavel(k)));
end

saveas(gcf, 'grafico_zonaEstavel.png')
