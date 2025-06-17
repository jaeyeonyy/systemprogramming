#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/wait.h>

#define MAX_NUMBERS 500

int *shmaddr;   // 공유 메모리 포인터

void handler(int signo) {
    // 숫자 개수 읽기
    int n = shmaddr[0];
    int sum = 0;
    // 입력받은 전체 숫자의 합을 계산
    // shmaddr[0]은 숫자의 개수이므로 제외
    for (int i = 1; i <= n; i++) {
        sum += shmaddr[i];
    }

    // 계산된 합을 출력
    printf("총합 = %d\n", sum);

    // 공유 메모리 연결 해제
    if(shmdt(shmaddr)== -1) {
        // 공유 메모리 연결 해제 실패
        perror("shmdt");
        exit(1);
    }
    exit(0);
}

int main(void) {
    // 공유 메모리 식별자 생성
    // 부모 자식간 메모리 공유 -> IPC_PRIVATE 사용
    // IPC_CREAT : 공유 메모리 새로 생성
    // 0666: 읽기/쓰기 권한 부여
    int shmid = shmget(IPC_PRIVATE, (MAX_NUMBERS + 1) * sizeof(int),
                   IPC_CREAT | 0666);
    if (shmid < 0) {
        // 공유 메모리 식별자 생성 실패
        perror("shmget");
        return 1;
    }

    pid_t pid = fork();
    switch(pid){
        case -1:
            // fork 실패
            perror("fork");
            // 공유 메모리 제거
            shmctl(shmid, IPC_RMID, NULL);
            return 1;
        case 0: // 자식 프로세스:프로세스2
            // 공유 메모리 연결
            shmaddr = shmat(shmid, NULL, 0);
            if (shmaddr == (void*)-1) {
                // 공유 메모리 연결 실패
                perror("shmat (child)");
                exit(1);
            }

            // sigset으로 핸들러 등록
            if (sigset(SIGUSR1, handler) == SIG_ERR) {
                // 핸들러 등록 실패
                perror("sigset");
                exit(1);
            }
            pause();  // 부모가 보낼 SIGUSR1 기다림
            break;
        default:
            // 부모 프로세스: 프로세스1
            char buf[4096];
            // 표준 입력에서 숫자 입력 받기
            // 숫자는 공백으로 구분됨
            ssize_t len = read(0, buf, sizeof(buf) - 1);
            if (len < 0) {
                // 표준 입력에서 읽기 실패
                perror("read");
                // 자식 프로세스에게 SIGKILL 보내고 정리
                kill(pid, SIGKILL);
                wait(NULL);
                // 공유 메모리 제거
                shmctl(shmid, IPC_RMID, NULL);
                return 1;
            }
            buf[len] = '\0';

            int tmp[MAX_NUMBERS], count = 0;
            // 입력받은 문자열을 공백과 줄바꿈으로 분리하고
            // atoi 함수를 사용하여 정수로 변환하여 tmp 배열에 저장
            char *tok = strtok(buf, " \n");
            while (tok != NULL) {
                tmp[count++] = atoi(tok);
                tok = strtok(NULL, " \n");
            }

            // 공유 메모리 연결
            shmaddr = shmat(shmid, NULL, 0);
            if (shmaddr == (void*)-1) {
                // 공유 메모리 연결 실패
                perror("shmat (parent)");
                // 자식 프로세스에게 SIGKILL 보내고 정리
                kill(pid, SIGKILL);
                wait(NULL);
                // 공유 메모리 제거
                shmctl(shmid, IPC_RMID, NULL);
                return 1;
            }
            // 공유 메모리 첫번째 칸에 숫자 개수 저장
            shmaddr[0] = count;
            // 공유 메모리 두번째 칸부터 숫자 저장
            for (int i = 0; i < count; i++) {
                shmaddr[i + 1] = tmp[i];
            }

            // 자식에게 SIGUSR1 보내고 대기
            kill(pid, SIGUSR1);
            wait(NULL);

            // 공유 메모리 연결 해제 및 제거
            if (shmdt(shmaddr) == -1) {
                perror("shmdt (parent)");
                return 1;
            }
            shmctl(shmid, IPC_RMID, NULL);
            break;
    }

    return 0;
}
