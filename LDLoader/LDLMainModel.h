#ifndef __LDLMAINMODEL_H__
#define __LDLMAINMODEL_H__

#include <LDLoader/LDLModel.h>
#include <TCFoundation/TCStlIncludes.h>

class LDLPalette;

typedef std::map<std::string, bool> StringBoolMap;

class LDLMainModel : public LDLModel
{
public:
	LDLMainModel(void);
	virtual bool load(const char *filename);
	virtual TCDictionary* getLoadedModels(void);
	virtual void print(void);
	virtual int getEdgeColorNumber(int colorNumber);
	virtual void getRGBA(int colorNumber, int& r, int& g, int& b, int& a);
	virtual bool hasSpecular(int colorNumber);
	virtual bool hasShininess(int colorNumber);
	virtual void getSpecular(int colorNumber, float *specular);
	virtual void getShininess(int colorNumber, float &shininess);
	virtual bool colorNumberIsTransparent(int colorNumber);
	virtual LDLPalette *getPalette(void) { return m_mainPalette; }
	virtual const LDLPalette *getPalette(void) const { return m_mainPalette; }
	virtual void setExtraSearchDirs(TCStringArray *value);
	TCStringArray *getExtraSearchDirs(void) { return m_extraSearchDirs; }
	virtual bool isMainModel(void) const { return true; }

	// Flags
	void setLowResStuds(bool value) { m_mainFlags.lowResStuds = value; }
	bool getLowResStuds(void) const { return m_mainFlags.lowResStuds; }
	void setBlackEdgeLines(bool value) { m_mainFlags.blackEdgeLines = value; }
	bool getBlackEdgeLines(void) const { return m_mainFlags.blackEdgeLines; }
	void setProcessLDConfig(bool value) { m_mainFlags.processLDConfig = value; }
	bool getProcessLDConfig(void) const
	{
		return m_mainFlags.processLDConfig != false;
	}
	void setSkipValidation(bool value) { m_mainFlags.skipValidation = value; }
	bool getSkipValidation(void) const
	{
		return m_mainFlags.skipValidation != false;
	}
	void setBoundingBoxesOnly(bool value)
	{
		m_mainFlags.boundingBoxesOnly = value;
	}
	bool getBoundingBoxesOnly(void) const
	{
		return m_mainFlags.boundingBoxesOnly != false;
	}
	void setSeamWidth(float value);
	float getSeamWidth(void) const { return m_seamWidth; }
	virtual void cancelLoad(void) { m_mainFlags.loadCanceled = true; }
	virtual bool getLoadCanceled(void)
	{
		return m_mainFlags.loadCanceled != false;
	}
	bool &ancestorCheck(const std::string &name) { return m_ancestorMap[name]; }
	void ancestorClear(const std::string &name) { m_ancestorMap[name] = false; }
	virtual TCObject *getAlertSender(void) { return m_alertSender; }
	virtual void setAlertSender(TCObject *value) { m_alertSender = value; }
private:
	//LDLMainModel(const LDLMainModel &other);
	virtual TCObject *copy(void) const;
protected:
	virtual void dealloc(void);
	virtual void processLDConfig(void);

	TCObject *m_alertSender;
	TCDictionary *m_loadedModels;
	LDLPalette *m_mainPalette;
	TCStringArray *m_extraSearchDirs;
	float m_seamWidth;
	struct
	{
		// Public flags
		bool lowResStuds:1;
		bool blackEdgeLines:1;
		bool processLDConfig:1;
		bool skipValidation:1;
		bool boundingBoxesOnly:1;
		// Semi-public flags
		bool loadCanceled:1;
	} m_mainFlags;
	StringBoolMap m_ancestorMap;
};

#endif // __LDLMAINMODEL_H__
