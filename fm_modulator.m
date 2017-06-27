%% Modulation
%% constants
clear all
close all 
pi = 3.1415926;
%% frequencies
fs = 48000; % sampling frequency
fc = 12000; % center frequency
Ts = 1/fs;
lut_table_size = 4096; % length for the cosine lookup table
nk=4;  % 4 waere besser
k = fc/nk; % frequency deviation 
f_out_max = fc+k;
f_out_min = fc-k;
lut_fak=lut_table_size/4;
debug = 0;
%% generate signal
samples = [1,50*fs];
[y,Fs] = audioread('inputfile.wav',samples);
signal = (y(400000:2400000,2))';
%plot(signal);
buffer_length = round(length(signal)/1000); % length of ping and pong buffers =500
%% generate cos_lut
cos_lut = 10000*cos(2*pi*(1:1:lut_table_size)/lut_table_size);

if debug == 1;
figure;
plot(1:1:lut_table_size,cos_lut)
title('Cosine lookup table');

end;
%% read input signal to buffer

inputbuffer = zeros (1,buffer_length);
outputbuffer = zeros (1,buffer_length);
buffer = 1;
pointer_old=0;  %pointer at the pass pointer(n-1)
for i = 1:1:buffer_length;
    inputbuffer(1,i) = 10*signal(i);  % ping -> pong
    delta_phi = 2*pi * Ts * (fc + k * inputbuffer(buffer,i));   % calculate the phase difference
    pointer_new = delta_phi/(2*pi)*lut_table_size;   % transfer to pointer
    pointer_min = lut_fak*(1-1/nk);         % just to look the value of pointer min[0:4800]
    pointer_corr = pointer_new + pointer_old; % pointer at the moment pointer(n)
    
    %% begin interpolation
    pointer_rest = pointer_corr - round(pointer_corr);  % the rest calculation
    pointer_corr_integer = round(pointer_corr);         % transfer the value of pointer (float) to integer
    %pointer_corr0 = mod(pointer_corr_rounded, lut_table_size);
    pointer_0 = mod(pointer_corr_integer, lut_table_size);
    pointer_1 = mod(pointer_corr_integer+1, lut_table_size);
    pointer_old = pointer_corr;
    outputbuffer(1,i) =cos_lut(pointer_0+1)+pointer_rest*(cos_lut(pointer_1+1)-cos_lut(pointer_0+1));
    % end interpolation
end;        


figure;
subplot(2,1,1)
plot(0:1:buffer_length-1,inputbuffer(1,:));
xlim([0 buffer_length-1]);
legend('ping-in');
title('last input buffer contents')
subplot(2,1,2)
plot(outputbuffer(1,:));
xlim([0 length(outputbuffer(1,:))+1]);
legend('ping-out');
title(['last output buffer contents f-out = [' ,num2str(f_out_min) ':' num2str(f_out_max) ']Hz'])

sig_to_audio = (outputbuffer(1,:)./32000);

audiowrite('modulated_sound.wav',sig_to_audio,48000)
sig_towrite = round(outputbuffer(1,:))';
fileID = fopen('modulated_2milion.txt','w');
fprintf(fileID,'%i\n',sig_towrite);
fclose(fileID);
