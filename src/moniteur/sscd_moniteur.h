#ifndef MONITEUR_H
#define MONITEUR_H

void handle_signal();
float read_cpu_usage();
float read_ram_usage();
float read_load_average();
void log_metrics(float cpu, float ram, float load);
void send_alert(float value);

#endif
