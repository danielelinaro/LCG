clear all;
close all;
clc;

%%
[entities,info] = loadH5Trace('recording.h5');
tfull = (0:length(entities(1).data)-1) * info.dt;
Vfull = entities(1).data; 
spks = find(Vfull == 50);
triggers = tfull(spks(100:100:end));
[entities,info] = loadH5Trace('triggered_recording.h5');
t = (0:size(entities(1).data,2)-1) * info.dt;
V = entities(1).data;

%%
cmap = jet(length(triggers));
figure;
hold on;
plot(tfull,Vfull,'k');
for k=1:length(triggers)
    plot(triggers(k)+t+info.dt, V(k,:), 'Color', cmap(k,:));
end
xlabel('Time (s)');
ylabel('Voltage (mV)');
axis tight;
axis([xlim,-80,60]);
