#ifndef MONITEUR_H
#define MONITEUR_H

void handle_signal(int sig);
float read_cpu_usage();
void log_metrics(float cpu_usage);
void send_alert(float value);

#endif
