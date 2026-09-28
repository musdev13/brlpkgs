#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sqlite3.h>

static long count_pacman(void) {
    DIR *dir = opendir("/var/lib/pacman/local");

    if (!dir)
        return 0;

    long count = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;

        if (entry->d_type == DT_DIR)
            count++;
    }

    closedir(dir);

    return count;
}

static long count_dpkg(void) {
    FILE *file = fopen(
        "/bedrock/strata/ubuntu/var/lib/dpkg/status",
        "r"
    );

    if (!file)
        return 0;

    long count = 0;
    char line[256];

    while (fgets(line, sizeof(line), file) != NULL) {
        if (strcmp(line, "Status: install ok installed\n") == 0)
            count++;
    }

    fclose(file);

    return count;
}

static long count_rpm(void) {
    const char *path =
        "/bedrock/strata/fedora/usr/lib/sysimage/rpm/rpmdb.sqlite";

    sqlite3 *db = NULL;
    sqlite3_stmt *stmt = NULL;
    long count = 0;

    if (sqlite3_open_v2(
            path,
            &db,
            SQLITE_OPEN_READONLY,
            NULL
        ) != SQLITE_OK) {
        goto cleanup;
    }

    if (sqlite3_prepare_v2(
            db,
            "SELECT COUNT(*) FROM Packages;",
            -1,
            &stmt,
            NULL
        ) != SQLITE_OK) {
        goto cleanup;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW)
        count = sqlite3_column_int64(stmt, 0);

cleanup:
    if (stmt)
        sqlite3_finalize(stmt);

    if (db)
        sqlite3_close(db);

    return count;
}

static long count_flatpak_dir(const char *path) {
    DIR *dir = opendir(path);

    if (!dir)
        return 0;

    long count = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;

        char full_path[4096];

        snprintf(
            full_path,
            sizeof(full_path),
            "%s/%s",
            path,
            entry->d_name
        );

        struct stat st;

        if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode))
            count++;
    }

    closedir(dir);

    return count;
}

static long count_flatpak(void) {
    long count = 0;

    count += count_flatpak_dir("/var/lib/flatpak/app");

    const char *home = getenv("HOME");

    if (home) {
        char path[4096];

        snprintf(
            path,
            sizeof(path),
            "%s/.local/share/flatpak/app",
            home
        );

        count += count_flatpak_dir(path);
    }

    return count;
}

int main(void) {
    const long apt = count_dpkg();
    const long pacman = count_pacman();
    const long rpm = count_rpm();
    const long flatpak = count_flatpak();

    printf(
        "%ld (apt), %ld (pacman), %ld (rpm), %ld (flatpak)\n",
        apt,
        pacman,
        rpm,
        flatpak
    );

    return 0;
}
