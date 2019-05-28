#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

char *initDir;
int faultCount = 0;

struct printFormat{
    char type;         // 파일 유형 - or d
    char mod[10];       // 파일 권한 rwx------
    int links;          // 링크 수
    char *owner;        // 소유주
    char *group;        // 그룹
    int fileSize;       // 파일 사이즈
    char lastChange[20];   // 마지막 변경 시간
    char *fileName;     // 파일 이름
    char *symbolPath;   // 심볼릭링크 경로
    char *symbol;       // 심볼링링크 화살표
};

void searchDirectory(char *curDirectory){

    /*
    
        현재 경로를 탐색.
        1. 현재 경로를 출력
        2. 전체 합계 출력
        3. 각 파일들을 이름 순으로 정렬해서 출력
        세부정보 양식에 맞춰서 출력한다.
        4. 다시 파일들을 이름 순으로 돌면서 폴더파일이면
        재귀호출하여 하위 폴더 출력한다.
    
    */

    // 무한루프 방지
    if(faultCount++ > 10){
        return;
    }

    DIR *pDir;

    if((pDir = opendir(curDirectory)) < 0){
        /* error */
        printf("DirectoryOpenFailed : %s\n", curDirectory);
        return;
    }

    // printf("DirectoryOpened : %s\n", curDirectory);

	// char curd[1024] = ".";
	// size_t isz = strlen(initDir);
	// size_t csz = strlen(curDirectory);
	// int sz = csz - isz;
    int i;
	// for (i = isz; i < csz; i++) {
    //     curd[i] = curDirectory[i];
    //     // strcat(curd, curDirectory[i]);
	// }
    // curd[i] = '\0';

    // printf("curd : %s, isz : %d, csz : %d", curd, isz, csz);

    struct dirent **cDir;
	int cnt = 0;

    if((cnt = scandir(curDirectory, &cDir, NULL, alphasort)) == -1){
        // 디렉터리 조회 오류
        printf("DirectoryScanFailed : %s\n", curDirectory);
        return;
    }
    
    struct stat stat;
    struct printFormat *pf = (struct printFormat *)malloc(cnt * sizeof(struct printFormat));

    // 현재 경로 변경
    chdir(curDirectory);

    int total = 0;      // 합계
    for(i = 0; i < cnt; i++){
        lstat(cDir[i]->d_name, &stat);

        // 파일 타입 판별
        if(S_ISDIR(stat.st_mode)){
            if(strcmp(".", cDir[i]->d_name) == 0
            || strcmp("..", cDir[i]->d_name) == 0){
                // . , .. 은 출력 안함
                continue;
            }
            pf[i].type = 'd';       // 디렉터리
        }else if(S_ISREG(stat.st_mode)){
            pf[i].type = '-';       // 일반 파일
        }

        // 심볼릭 링크
        if(S_ISLNK(stat.st_mode)){
            pf[i].type = 'l';
            char *realPath = (char*)malloc(BUFSIZ * sizeof(char));
            pf[i].symbolPath = (char*)malloc(BUFSIZ *sizeof(char));
            pf[i].symbol = "->";
            realpath(cDir[i]->d_name, pf[i].symbolPath);
        }else{
            pf[i].symbol = "";
            pf[i].symbolPath = "";
        }

        // 유저 권한 판별
        if((stat.st_mode & S_IREAD) != 0){
            pf[i].mod[0] = 'r';
        }else{
            pf[i].mod[0] = '-';
        }
        if((stat.st_mode & S_IWRITE) != 0){
            pf[i].mod[1] = 'w';
        }else{
            pf[i].mod[1] = '-';
        }
        if((stat.st_mode & S_IEXEC) != 0){
            pf[i].mod[2] = 'x';
        }else{
            pf[i].mod[2] = '-';
        }

        // 그룹 권한 판별
        if((stat.st_mode & (S_IREAD >> 3)) != 0){
            pf[i].mod[3] = 'r';
        }else{
            pf[i].mod[3] = '-';
        }
        if((stat.st_mode & (S_IWRITE >> 3)) != 0){
            pf[i].mod[4] = 'w';
        }else{
            pf[i].mod[4] = '-';
        }
        if((stat.st_mode & (S_IEXEC >> 3)) != 0){
            pf[i].mod[5] = 'x';
        }else{
            pf[i].mod[5] = '-';
        }

        // 기타 권한 판별
        if((stat.st_mode & S_IROTH) != 0){
            pf[i].mod[6] = 'r';
        }else{
            pf[i].mod[6] = '-';
        }
        if((stat.st_mode & S_IWOTH) != 0){
            pf[i].mod[7] = 'w';
        }else{
            pf[i].mod[7] = '-';
        }
        if((stat.st_mode & S_IXOTH) != 0){
            pf[i].mod[8] = 'x';
        }else{
            pf[i].mod[8] = '-';
        }

        pf[i].mod[9] = '\0';

        // 링크 수
        pf[i].links = stat.st_nlink;

        // 소유주
        struct passwd *fpwd = getpwuid(stat.st_uid);
        pf[i].owner = fpwd->pw_name;

        // 그룹명
        struct group *fpgp = getgrgid(stat.st_gid);
        pf[i].group = fpgp->gr_name;

        // 파일 사이즈 계산
        pf[i].fileSize = stat.st_size;

        // 시간 계산
        char s[20];
        time_t curTime = stat.st_mtime;
        struct tm *t = localtime(&curTime);
        sprintf(pf[i].lastChange, "%04d-%02d-%02d %02d:%02d", t->tm_year + 1900
        , t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min);
        //pf[i].lastChange = s;

        pf[i].fileName = cDir[i]->d_name;

        // total 사이즈 계산
        total += stat.st_blocks;

    }

    // 출력
    // 현재 디렉터리 출력
	printf("%s:\n", curDirectory);
    // 합계
    printf("합계 %d\n", total/2);
    for(i = 2; i < cnt; i++){
        printf("%c%s %d %s %s %8d %s %s %s %s\n", pf[i].type, pf[i].mod, pf[i].links, pf[i].owner, pf[i].group, pf[i].fileSize, pf[i].lastChange, pf[i].fileName, pf[i].symbol, pf[i].symbolPath);            
    }

    // 다음 폴더 재귀 탐색
    for(i = 2; i < cnt; i++){
        if(pf[i].type == 'd'){
            // 디렉터리이면 하위 탐색
            char *newDir = (char*)malloc(1024*sizeof(char));
            strcpy(newDir, curDirectory);
            strcat(newDir, "/");
            strcat(newDir, pf[i].fileName);
            //printf("next : %s\n", newDir);
            printf("\n");
            searchDirectory(newDir);
            free(newDir);
        }
    }


}

int main(){

	char *buf;
	size_t size = 1024;
	initDir = getcwd(NULL, 1024);
	searchDirectory(initDir);

    return 0;
}