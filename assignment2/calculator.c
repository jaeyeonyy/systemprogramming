#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

// 숫자를 표현하는 문자열 노드 구조체(연결리스트)
typedef struct NumStringNode {
    char *numString;
    struct NumStringNode* next;
} NumStringNode;

// 연산자 스택 노드 구조체(스택)
typedef struct OperandNode {
    char op;
    struct OperandNode *next;
} OperandNode;

// 실수형 스택 노드 구조체(스택)
typedef struct DoubleNode {
    double value;
    struct DoubleNode *next;
} DoubleNode;

// 연산자 스택 push 함수
void pushOperand(OperandNode **top, const char op) {
    OperandNode *newNode = (OperandNode *)malloc(sizeof(OperandNode));
    newNode->op = op;
    newNode->next = *top;
    *top = newNode;
}

// 연산자 스택 pop 함수
char popOperand(OperandNode **top) {
    if (*top == NULL) return '\0';
    OperandNode *temp = *top;
    char op = temp->op;
    *top = (*top)->next;
    free(temp);
    return op;
}
// 연산자 스택 peek 함수
char peek(OperandNode *top) {
    if(top != NULL)
        return top->op;
    else
        return '\0';
}

// 연산자의 우선순위 비교
int precedence(char op) {
    if (op == '*' || op == '/') return 2;
    if (op == '+' || op == '-') return 1;
    return -1;
}

// 연산자인지 판별
int isOperator(char *str) {
    if(str == NULL) return -1;
    if(strlen(str) != 1) return 0;
    char c = *str;
    return (c == '+' || c == '-' || c == '*' || c == '/');
}

// 실수형 스택 push 함수
void pushDouble(DoubleNode **top, double value) {
    DoubleNode *newNode = (DoubleNode *)malloc(sizeof(DoubleNode));
    newNode->value = value;
    newNode->next = *top;
    *top = newNode;
}

// 실수형 스택 pop 함수
double popDouble(DoubleNode **top) {
    if (*top == NULL) return 0.0; 
    DoubleNode *temp = *top;
    double value = temp->value;
    *top = temp->next;
    free(temp);
    return value;
}

// numStringNode를 연결리스트에 추가하는 함수
NumStringNode* appendNode(NumStringNode **head, char *numString) {
    NumStringNode *newNode = malloc(sizeof(NumStringNode));
    if (!newNode) {
        perror("malloc failed");
        exit(1);
    }

    size_t len = strlen(numString);
    newNode->numString = malloc(len + 1);
    if (!newNode->numString) {
        perror("malloc failed");
        exit(1);
    }
    strcpy(newNode->numString, numString);
    newNode->next = NULL;

    if (*head == NULL) {
        *head = newNode;
    } else {
        NumStringNode *cur = *head;
        while (cur->next != NULL)
            cur = cur->next;
        cur->next = newNode;
    }

    return *head;
}


// 중위 → 후위 표기 변환 함수
NumStringNode* infixToPostfix(char *expr) {
    NumStringNode *postfix = NULL;
    OperandNode *opStack = NULL;

    // 공백을 기준으로 문자열 분리
    char *token = strtok(expr, " ");
    while (token != NULL) {
        if (isOperator(token)) {
            // 연산자일 경우 스택에서 pop하여 후위식에 추가
            char currentOp = token[0];

            // 스택이 비어있지 않고, 현재 연산자의 우선순위가 낮거나 같을 경우
            // 스택에서 pop하여 후위식에 추가
            while (opStack != NULL && precedence(peek(opStack)) >= precedence(currentOp)) {
                char poppedOp[2] = { popOperand(&opStack), '\0' };
                appendNode(&postfix, poppedOp);
            }
            pushOperand(&opStack, currentOp);
        } else {
            // 숫자일 경우 후위식에 추가
            appendNode(&postfix, token); 
        }
        token = strtok(NULL, " ");
    }

    // 스택에 남은 연산자들 후위식으로 추가
    while (opStack != NULL) {
        char poppedOp[2] = { popOperand(&opStack), '\0' };
        appendNode(&postfix, poppedOp);
    }

    return postfix
}

// 후위 표기법 계산 함수
double evalPostfix(NumStringNode *postfix) {
    DoubleNode *stack = NULL;

    while (postfix != NULL) {
        // 숫자, 연산자를 하나의 토큰으로 간주
        char *token = postfix->numString;

        if (!isOperator(token)) {
            // 숫자일 경우 문자열 -> 실수로 변환 후 스택에 push
            double num = atof(token);
            pushDouble(&stack, num);
        } else {
            // 연산자일 경우 스택에서 두 개의 피연산자 pop 후 계산
            // 결과를 다시 스택에 push
            double b = popDouble(&stack);
            double a = popDouble(&stack);
            double res = 0;

            switch (token[0]) {
                case '+': res = a + b; break;
                case '-': res = a - b; break;
                case '*': res = a * b; break;
                case '/': res = a / b; break;
            }
            pushDouble(&stack, res);
        }

        postfix = postfix->next;
    }

    return popDouble(&stack);
}

void freeNumStringList(NumStringNode *head) {
    while (head) {
        NumStringNode *temp = head;
        head = head->next;
        free(temp->numString);  // 문자열 해제
        free(temp);             // 노드 해제
    }
}


int main(int argc, char *argv[]){
    
    // 명령행 인자로 받은 파일 경로를 사용
    char path[BUFSIZ];
    char str[BUFSIZ];
    strcpy(path, argv[1]);
    
    // 전달받은 파일명으로 파일 열기
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("파일 열기 실패");
        exit(1);
    }

    // 파일 내용 읽기
    ssize_t size = read(fd, str, BUFSIZ);
    if (size == -1) {
        perror("파일 읽기 실패");
        close(fd);
        exit(1);
    }
    // 문자열 끝에 널 문자 추가
    str[size] = '\0';
    close(fd);


    // 사칙연산 문자열을 후위 표기법으로 변환
    NumStringNode *postfix = infixToPostfix(str);

    // 후위 표기법을 기반으로 수식 계산
    double result = evalPostfix(postfix);
    printf("계산 결과: %.2lf\n", result);
    // 메모리 해제
    freeNumStringList(postfix);
    
    exit(0);
}