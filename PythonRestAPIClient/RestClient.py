import requests

with requests.Session() as session:
    session.auth = ('caddok', '')

    # preset attributes for the new document.
    # Note: z_nummer / z_index are not given, the system will assign them
    # automatically
    # Note: cdb_classname MUST be given, because the document class lives in a
    # class hierarchy!
    create_attrs = {"temperatur": 22,
                    "luftfeuchtigkeit": 66,
                    "laengengrad": "Laengengrad...",
                    "breitengrad": "Breitengrad...",
                    "datum": "2016-08-22T20:00:00",
                    "cdb_classname": "cdb_sensordaten"}
    # by giving the "json" argument to post, we tell Requests to do the JSON
    # encoding of the dictionary
    resp = session.post("http://localhost/api/v1/collection/cdb_sensordaten",
                        json=create_attrs)
    if resp.status_code == 200:
        # on success, we get the complete data for the new object
        data = resp.json()
        print ("Created data %s/%s"
               % (data['temperatur'], data['luftfeuchtigkeit']))
    else:
        print ("Error: %s" % resp.status_code)

    resp = session.get('http://localhost/server/__quit__')