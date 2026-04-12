#ifndef __TASKOBSERVER_H__
#define __TASKOBSERVER_H__

class TaskObserver
{
public:
	virtual void OnTaskFinished() = 0;
	virtual void OnTaskFailed() = 0;
	virtual ~TaskObserver() = default;
};



#endif // __TASKOBSERVER_H__
