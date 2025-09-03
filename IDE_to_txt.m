clear all, clc

arduinoSerial = serialport('COM11', 9600); % Ajuste COM e baudrate
flush(arduinoSerial); % Limpa buffer da porta serial
time = datetime('now');
d = day(time);

N = 100;
CO2 = nan(1, N);
TVOC = nan(1, N);
idx = 1;
mediaCO2 = 1e6;
mediaTVOC = 1e6;

while true
    
    if arduinoSerial.NumBytesAvailable > 0
        linha = strtrim(readline(arduinoSerial));
        nome = ['Teste_sensor_quimico_' num2str(d) '.txt'];
        texto = fopen(nome, 'a');
        time = datetime('now');
        h = hour(time);
        m = minute(time);
        fprintf(texto, '%.0f:%.0f - %s\n',h,m,linha);
        fprintf('%.0f:%.0f - %s\n',h,m,linha);
        fclose(texto);
        tokens = regexp(linha, 'eCO2:\s*(\d+)\s*ppm,\s*TVOC:\s*(\d+)', 'tokens');
        if ~isempty(tokens)
            vals = str2double(tokens{1});
            valCO2 = vals(1);
            valTVOC = vals(2);
            CO2(idx) = valCO2;
            TVOC(idx) = valTVOC;
            idx = idx + 1;
    
            if idx > N
                idx = 1; 
            elseif idx == N
                mediaNCO2 = mean(CO2);
                mediaNTVOC = mean(TVOC);
                if (abs(mediaCO2-mediaNCO2) <= 10 && abs(mediaTVOC-mediaNTVOC) <= 10)
                    writeline(arduinoSerial,"Calibrado");
                end
                mediaCO2 = mediaNCO2;
                mediaTVOC = mediaNTVOC;
            end
        end
    end             
end