블로그 빌드한 과정
먼저 자신의 github 계정에서 블로그로 사용하게 될 레퍼토리를 만들었다. 레퍼토리를 만든 후 jekyll 웹페이지 생성기를 사용하기 위해 Ruby를 설치하였다. 그 후 cmd를 통해 자신의 서버에서 블로그를 만들기 위해 bundle 명령어를 입력하여 필요한 파일을 다운한 후 bundle exec jekyll serve 명령어를 입력하여 나온 주소를 인터넷에 복사 붙여넣기 하여 블로그가 잘 나오는 것을 확인하게 되었다. 하지만 우리의 목표는 github page를 만드는 것이기 때문에, 자신의 컴퓨터에서 블로그를 생성하기 위한 파일에 자신의 git 주소를 연결시켜 git add, git commit -m “”, git push -u origin master 등을 통해 레퍼토리로 커밋한 후 github page에서 블로그가 적용되게 하였다.
다음은 밋밋한 초기 블로그에 테마를 입히기 위해 사이트로 들어가 자신이 원하는 테마를 찾아본다. 나는 moonwalk 테마를 사용하였다. 원하는 테마를 고른 후 그 테마가 있는 git 주소를 블로그 생성 파일에 연결 시킨 후 그 테마를 적용시키기 위해 _config.yml은 삭제한 후 _post파일은 남겨 놓는 등 파일을 선별한 후 덮어쓰기 하고 cmd에서 적용시킨 것을 확인한 후 마찬가지로 commit을 통해 적용시켰다.
3번째, 초기의 블로그 세팅에는 나에게 필요없는 부분들이 있다. 그것은 _config.yml파일을 visual studio를 통해 필요없는 부분은 삭제한 후 필요한 것은 이름을 고치고 url또한 적절하게 설정하였다.
4번째, favicon을 적용시키기 위해 자신의 원하는 사진을 ico 확장자로 변환시켜주는 사이트를 통해 변환시킨 후 __asset 파일에 집어 넣은 후 head 부분의 custom favicon에 자신이 원하는 사진의 파일이 있는 디렉토리를 설정하였다.


# README.md

__먼저 자신만의 github 계정에서 자신의 개인 repository를 만듭니다.__
__그 다음 local repository와의 연동을 위해 우분투 터미널에 git clone <repo_name><path>를 입력합니다.__
__그런 후 자신의 github에 build하고 싶은 프로젝트 파일을 선택합니다.__
__우분투 터미널에 들어가서 git status를 통해 현재 git 상태를 확인한 후 git add 명령을 입력하여 tracked 상태로 만듭니다.__
__git commit -m "msg"로 커밋을 남깁니다.__
__git branch -M main으로 현재 branch의 이름을 main로 변경합니다.__
__그러나 현재 github의 바뀐 정책에 따라 Token를 생성해야 하기에 github 홈페이지에 가서 Token을 적절한 용도에 맞게 생성합니다.__
__Password에 PAT를 입력하여 git push를 실행시킵니다.__
