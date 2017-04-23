import cherrypy
import json
import unicodedata
import cangjie
import simplepinyin

class CloudInput(object):
    def index(self):
        return "Cloud Input Method"
    index.exposed = True

    def cangjie(self, *args, **kwds):
        cb = kwds.get("cb")
        text = kwds.get("text")
        if not text:
            return "Cangjie Input Method"

        CJ = cangjie.CangJie(cangjie.versions.CANGJIE3, cangjie.languages.COMMON)
        candidates = [ch.chchar for ch in CJ.getCharacters(text)]
        result = [text, candidates]
        headers = cherrypy.response.headers

        if cb:
            headers['Content-Type'] = "application/javascript;charset=utf-8"
            return (cb+"("+json.dumps(result, ensure_ascii=False)+")").encode()
        else:
            headers['Content-Type'] = "application/json;charset=utf-8"
            return json.dumps(result, ensure_ascii=False).encode()
    cangjie.exposed = True

    def pinyin(self, *args, **kwds):
        cb = kwds.get("cb")
        text = kwds.get("text")
        if not text:
            return "Pinyin Input Method"

        k = 0
        for i in range(0, len(text)):
            if not unicodedata.name(text[i]).startswith("CJK"):
                k = i
                break
        prefix = text[:k]
        pinyin = text[k:]

        PY = simplepinyin.SimplePinyin()
        candidates, match_lens = PY.convert(pinyin, prefix)
        result = [text, candidates, match_lens]
        headers = cherrypy.response.headers

        if cb:
            headers['Content-Type'] = "application/javascript;charset=utf-8"
            return (cb+"("+json.dumps(result, ensure_ascii=False)+")").encode()
        else:
            headers['Content-Type'] = "application/json;charset=utf-8"
            return json.dumps(result, ensure_ascii=False).encode()
    pinyin.exposed = True

cherrypy.config["server.socket_host"] = "0.0.0.0"
cherrypy.config["tools.encode.on"] = True
cherrypy.config["tools.encode.encoding"] = "utf-8"
cherrypy.quickstart(CloudInput())
