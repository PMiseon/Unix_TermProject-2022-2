# Unix_TermProject_2022-2
2022년2학기 유닉스 Term project


# 프로젝트 소개 
계산 및 I/O 속도를 높이기 위한 병렬작업(parallel operation)을 구현하기 

![image](https://github.com/PMiseon/Unix_TermProject-2022-2/assets/106222104/4598b6f8-2752-40b3-80d0-af4314f49254)


## Basic parellel I/O
- 각 compute node가 주어진 데이터들을 개별로 I/O node에 전송하는 방법

## Client-oriented collective I/O 
- 각 client(compute node)가 주어진 영역의 데이터들을 다른 client로부터 모아서 연속적인 데이터들을 보내는 방법

## Server-oriented collective I/O
- 각 server(I/O node)가 주어진 영역의 데이터들을 client로부터 모아서 저장하는 방법 

