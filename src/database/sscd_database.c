#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

#define LOG_FILE "src/moniteur/moniteur_log.csv"
#define ROTATION_THRESHOLD 10 // nombre de lignes max
#define ARCHIVE_DIR "archives"

void rotate_log_if_needed()
{
    FILE *fp = fopen(LOG_FILE, "r");
    if (!fp)
        return;

    int line_count = 0;
    char line[256];

    while (fgets(line, sizeof(line), fp))
    {
        line_count++;
    }
    fclose(fp);

    if (line_count >= ROTATION_THRESHOLD)
    {
        printf("[Database] Rotation des logs nécessaire (%d lignes)\n", line_count);
        // Créer dossier si inexistant
        mkdir(ARCHIVE_DIR, 0755);

        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char archive_name[256];
        strftime(archive_name, sizeof(archive_name), ARCHIVE_DIR "/log_%Y%m%d%H%M%S.csv", tm_info);

        rename(LOG_FILE, archive_name); // déplacer

        // Recréer un fichier vide
        fp = fopen(LOG_FILE, "w");
        if (fp)
            fclose(fp);

        printf("[Database] Rotation effectuée → %s\n", archive_name);
    }
}

int main()
{
    while (1)
    {
        rotate_log_if_needed();
        sleep(30); // check toutes les 30 secondes
    }
    return 0;
}
