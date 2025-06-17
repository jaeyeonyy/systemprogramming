typedef struct{
    char name[20];
    int score;
} student;

typedef struct{
    long mtype;
    student data;
} message_student;

typedef struct{
    long mtype;
    float avg_score;
} message_avg_score;

typedef struct{
    long mtype;
    int command;
} message_command;

// MSG TYPE 정의
#define MSG_TYPE_SEND_STUDENT 1
#define MSG_TYPE_CMD 2
#define MSG_TYPE_RESPONSE_MAX_SCORE 3
#define MSG_TYPE_RESPONSE_AVG_SCORE 4


// CMD TYPE 정의
#define CMD_TYPE_GET_MAX_SCORE 11
#define CMD_TYPE_GET_AVG_SCORE 12
#define CMD_TYPE_EXIT 13

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <unistd.h> 

int main(int argc, char *argv[]){
    key_t key;
    int msgid;
    message_student msg_student;
    message_avg_score msg_avg_score;
    message_command msg_command;

    // 키 생성
    key = ftok("keyfile", 1);
    if(key == -1) {
        // 키 생성 실패 처리
        perror("ftok");
        exit(1);
    }
    // 메시지 큐 생성
    msgid = msgget(key, IPC_CREAT|0644);
    if (msgid == -1) {
        // 메시지 큐 생성 실패 처리
        perror("msgget");
        exit(1);
    }


    while (1) {
        // 입력받기 용 임시 버퍼
        char buf[40];
        printf("학생 이름과 점수를 입력하세요. (종료하려면 엔터만 입력하세요.)\n");
        int len=read(0, buf, sizeof(buf)-1);
        if(len == -1) {
            // 입력 실패 처리
            perror("read");
            exit(1);
        }

        buf[len] = '\0';  // 줄바꿈 제거

        // 첫 번째 토큰(이름) 추출
        char *token = strtok(buf, " \n");
        if (!token) {
            // 엔터만 입력한 경우 입력 종료
            printf("학생 데이터 입력이 종료되었습니다.\n");
            break;
        }
        // 이름 복사
        strcpy(msg_student.data.name, token);

        // 두 번째 토큰(점수) 추출
        token = strtok(NULL, " \n");
        if (!token) {
            printf("점수가 입력되지 않았습니다. 다시 입력해주세요.\n");
            continue;
        }
        msg_student.data.score = atoi(token);
        msg_student.mtype = MSG_TYPE_SEND_STUDENT;
        // 메시지 큐에 학생 데이터 전송
        if (msgsnd(msgid, &msg_student, sizeof(msg_student.data), 0) == -1) {
            // 메시지 전송 실패 처리
            perror("msgsnd");
            exit(1);
        }
    }


    int loop = 1;
    while(loop){
        printf("명령어를 입력하세요 (1: 최대 점수, 2: 평균 점수, 3: 종료): ");
        int command;
        scanf("%d", &command);
        // 명령 메시지 설정
        msg_command.mtype = MSG_TYPE_CMD;
        switch(command) {
            case 1:
                // 최대 점수 요청 명령 설정
                msg_command.command = CMD_TYPE_GET_MAX_SCORE;
                // 메시지 큐에 명령어 전송
                if(msgsnd(msgid, &msg_command, sizeof(msg_command.command), 0) == -1) {
                    // 메시지 전송 실패 처리
                    perror("msgsnd");
                    exit(1);
                }
                // 최대 점수 응답 메시지 수신
                if(msgrcv(msgid, &msg_student, sizeof(msg_student) - sizeof(long), MSG_TYPE_RESPONSE_MAX_SCORE, 0) == -1) {
                    // 메시지 수신 실패 처리
                    perror("msgrcv");
                    exit(1);
                }
                // 최대 점수를 가지는 학생의 정보를 출력
                printf("이름: %s 최대 점수: %d\n", msg_student.data.name, msg_student.data.score);
                break;
            case 2:
                // 평균 점수 요청 명령 설정
                msg_command.command = CMD_TYPE_GET_AVG_SCORE;
                // 메시지 큐에 명령어 전송
                if(msgsnd(msgid, &msg_command, sizeof(msg_command.command), 0) == -1) {
                    // 메시지 전송 실패 처리
                    perror("msgsnd");
                    exit(1);
                }
                // 평균 점수 응답 메시지 수신
                if(msgrcv(msgid, &msg_avg_score, sizeof(msg_avg_score) - sizeof(long), MSG_TYPE_RESPONSE_AVG_SCORE, 0) == -1) {
                    // 메시지 수신 실패 처리
                    perror("msgrcv");
                    exit(1);
                }
                // 평균 점수 출력
                printf("평균 점수: %.2f\n", msg_avg_score.avg_score);
                break;
            case 3:
                // 종료 명령 설정
                msg_command.command = CMD_TYPE_EXIT;
                // 메시지 큐에 종료 명령어 전송
                if(msgsnd(msgid, &msg_command, sizeof(msg_command.command), 0) == -1) {
                    // 메시지 전송 실패 처리
                    perror("msgsnd");
                    exit(1);
                }
                loop = 0;
                break;
            default:
                printf("잘못된 명령어입니다.\n");
                continue;
        }
    }

    // Server가 메시지 읽을 시간을 주기 위해 잠시 대기
    sleep(2);
    // 메시지 큐 제거
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        // 메시지 큐 제거 실패 처리
        perror("msgctl");
        exit(1);
    }
    printf("클라이언트 프로그램이 종료됩니다.");
    return 0;
}