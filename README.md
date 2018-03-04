# Curl-Http
C++ wrapper for  curl http  part, make curl more easily to ues


# Curl-Http
给Curl增加C++ Wrapper,当前支持Http请求。

## 特色
提供类似于NodeJS的req,res,callback形式的Http异步请求接口，并且用户不用管理内存。

## 例子


    enum RequestType {
	    SOFTWARE_TRATEGY,
	    NEWEST_VERSION,
	    PULL_PACKAGE,
	    USER_INFO,
	    PACKAGE_LIST,
	    QRCODE,
	    QRCODE_STATUS
    };
    //异步回调
    class Action : public Http::Action {
    private:
    //请求结果回调
	virtual void Do(const Http::Task&task) override {
	        //您传入的自定义数据
	        task.UserData();
	        //请求结束时curl返回的状态
	        task.CurlCode();
	        //服务器返回的数据和数据大小
	        task.MemoryAddr(),task.Size();
	    };
    //数据接收进度	    
    virtual int Progress(double totaltime, double dltotal, double dlnow, double ultotal, double ulnow) override{ return 0; }	    
    };
    //用户数据
    class UserData {
    public:
	    UserData(RequestType reqType) :m_requestType(reqType) { }
	    RequestType m_requestType;
    };
    std::string url="www.baidu.com";
    //UserData将在Http::Action::Do结束后帮你释放掉，
    //Action将在程序结束后被动释放或自己主动释放
    //建议Action单例，这样可以统一进行数据处理
    ROUTER.Get(url, new Action, new UserData);
    
    
