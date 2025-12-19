csv_path = 'stability_timeseries.csv';
opts = detectImportOptions(csv_path, 'Delimiter', ';');
df = readtable(csv_path, opts);

time_col = df.Properties.VariableNames{1};
tempo = df.(time_col);
tempo_h = tempo / 3600;

if any(strcmp(df.Properties.VariableNames, 'CarriedErlangs'))
    data_col = 'CarriedErlangs';
else
    cols = df.Properties.VariableNames;
    idx = find(contains(lower(cols), 'erlang'), 1);
    data_col = cols{idx};
end
valor = df.(data_col);

window_size = 50;
sd = movstd(valor, window_size);

threshold = 0.01 * max(valor);
stable = sd < threshold;

%% Find continuous stable intervals
min_duration = 100;

stable_bin = zeros(size(stable));
label_counter = 0;
in_region = false;

for i = 1:length(stable)
    if stable(i) && ~in_region
        label_counter = label_counter + 1;
        in_region = true;
    elseif ~stable(i) && in_region
        in_region = false;
    end
    if in_region
        stable_bin(i) = label_counter;
    end
end

start_stable = [];
end_stable = [];

for i = 1:max(stable_bin)
    idx = find(stable_bin == i);
    if numel(idx) >= min_duration
        start_stable = [start_stable; idx(1)];
        end_stable = [end_stable; idx(end)];
    end
end

figure;
plot(tempo_h, valor, 'b', 'LineWidth', 1.2); hold on;
xlim([0 24]);
xticks(0:3:24);
xlabel('Tempo [h]');
ylabel(data_col);
title('zona estável de erlangs');

for k = 1:length(start_stable)
    xpatch = tempo_h(start_stable(k):end_stable(k));
    ypatch = valor(start_stable(k):end_stable(k));
    patch([xpatch; flipud(xpatch)], ...
          [ypatch; flipud(ypatch)*0], ...
          [0.15 0.7 0.2], ...
          'FaceAlpha', 0.6, 'EdgeColor', [0 0.35 0]);
end

legend('Erlangs', 'Zona Estável');
grid on;
axis padded;

for k = 1:length(start_stable)
    fprintf('Zone %d: from t = %.2f to t = %.2f hours\n', ...
        k, tempo_h(start_stable(k)), tempo_h(end_stable(k)));
end

script_dir = fileparts(mfilename('fullpath'));
out_path = fullfile(script_dir, 'grafico_zonaEstavel_rede.png');
saveas(gcf, out_path);
