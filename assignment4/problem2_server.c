// 학생 구조체 정의 
typedef struct{
    char name[20];
    int score;
} student;

// 메시지 구조체 정의
// 학생 데이터 송수신용
typedef struct{
    long mtype;
    student data;
} message_student;

// 평균 점수 송수신용
typedef struct{
    long mtype;
    float avg_score;
} message_avg_score;

// 명령어 송수신용
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
    // 메시지 버퍼 선언
    message_student msg_student;
    message_avg_score msg_avg_score;
    message_command msg_command;

    // 학생 리스트 기본 크기 설정 및 현재 학생 수 초기화
    int list_size = 10;
    int count = 0;
    student *student_list = (student *)malloc(list_size * sizeof(student));
    if (student_list == NULL) {
        // 메모리 할당 실패 처리
        perror("malloc");
        exit(1);
    }

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
    int loop = 1;
    while(loop) {
        // 특정 메시지 타입이 있는 지 확인
        // 메시지 타입이 있으면 수신
        // 메시지 타입이 없으면 즉시 return

        // 학생 데이터 수신
        if(msgrcv(msgid, &msg_student, sizeof(msg_student.data), MSG_TYPE_SEND_STUDENT, IPC_NOWAIT) != -1) {
            printf("Client로부터 학생 데이터를 수신했습니다.\n");
            printf("학생 이름: %s, 점수: %d\n", msg_student.data.name, msg_student.data.score);
            // 학생 리스트 크기가 부족하면 재할당
            if(count >= list_size) {
                list_size += 10;
                student *new_list = (student *)realloc(student_list, list_size * sizeof(student));
                if (new_list == NULL) {
                    perror("realloc");
                    free(student_list);
                    exit(1);
                }
                student_list = new_list;
            }
            // 학생 데이터를 리스트에 추가
            student_list[count] = msg_student.data;
            count++;
        }
        // 명령어 수신
        if(msgrcv(msgid, &msg_command, sizeof(msg_command) - sizeof(long), MSG_TYPE_CMD, IPC_NOWAIT) != -1) {
            switch(msg_command.command) {
                case CMD_TYPE_GET_MAX_SCORE: {
                    // 최대 점수 요청 처리
                    printf("Client로부터 최대 점수 요청을 수신했습니다.\n");
                    int index = 0;
                    int max_score = student_list[0].score;
                    // 학생 리스트를 순회하여 최대 점수를 가지는 학생의 index 찾기
                    for (int i = 1; i < count; i++) {
                        if (student_list[i].score > max_score) {
                            max_score = student_list[i].score;
                            index = i;
                        }
                    }
                    printf("최대 점수를 가진 학생: %s, 점수: %d\n", student_list[index].name, student_list[index].score);
                    // 최대 점수를 가지는 학생의 정보를 응답 메시지에 설정
                    msg_student.mtype = MSG_TYPE_RESPONSE_MAX_SCORE;
                    msg_student.data = student_list[index];
                    // 최대 점수 응답 메시지 전송
                    if (msgsnd(msgid, &msg_student, sizeof(msg_student.data), 0) == -1) {
                        perror("msgsnd");
                        exit(1);
                    }
                    break;
                }
                case CMD_TYPE_GET_AVG_SCORE: {
                    // 평균 점수 요청 처리
                    printf("Client로부터 평균 점수 요청을 수신했습니다.\n");
                    int total_score = 0;
                    // 학생 리스트를 순회하여 총 점수 계산
                    for (int i = 0; i < count; i++) {
                        total_score += student_list[i].score;
                    }
                    // 평균 점수 계산
                    float avg_score =(float)total_score / count;
                    // 평균 점수 응답 메시지 설정
                    msg_avg_score.mtype = MSG_TYPE_RESPONSE_AVG_SCORE;
                    msg_avg_score.avg_score = avg_score;
                    printf("총 학생 수: %d, 총 점수: %d, 평균 점수: %.2f\n", count, total_score, avg_score);
                    // 평균 점수 응답 메시지 전송
                    if (msgsnd(msgid, &msg_avg_score, sizeof(msg_avg_score.avg_score), 0) == -1) {
                        perror("msgsnd");
                        exit(1);
                    }
                    break;
                }
                case CMD_TYPE_EXIT:
                    // 종료 요청 처리
                    printf("Client로부터 종료 요청을 수신했습니다.\n");
                    loop = 0;
                    break;
            }
        }
        sleep(1); // 메시지 수신 대기
    }
    // 학생 리스트 메모리 해제
    printf("서버가 종료됩니다.\n");
    free(student_list);
    return 0;
}
