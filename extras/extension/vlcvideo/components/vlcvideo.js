/*
    * The embedded VLC player gets totally destroyed even if merely hidden
      (or removed from the DOM), and totally reinitialised on reshow. This
      means that all state is lost if this happens.
*/


let Cu = Components.utils;
let Ci = Components.interfaces;
let Cc = Components.classes;
let Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function GM_log(str) {
  dump("[VLCVideo] " + str + "\n");
}

function VLCVideo() {}

VLCVideo.prototype = {

  classID: Components.ID("{7c4020fc-40fa-43a4-9b13-d7ba62171ce1}"),
  contractID: "@videolan.org/vlcvideo;1",
  classDescription: "VLC video",

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),
  
  observe: function (subject, topic, data)
  {
    switch (topic) {
      case "profile-after-change":
        Services.obs.addObserver(this, "content-document-global-created", false);
        break;
      case "content-document-global-created": {
        let window = subject;
        let document = window.wrappedJSObject.document;
        this.inject(document);
        break;
      }
    }
  },

  inject: function (document) {
    GM_log("injecting");
    (function () {
      // these are the DOM objects that we have to keep an eye on
      var vlcs = [];

      // these are the original functions that we later override
      var _createElement = document.createElement;

      function replaceStaticMedia(video) {
        GM_log('replacing static video');

        let embed = _createElement("embed");
        let videoAttrs = video.attributes;
        for (let i = 0; i < videoAttrs.length; ++i) {
          embed.setAttribute(videoAttrs[i].name, videoAttrs[i].value);
        }
        embed.setAttribute("type",        "application/x-vlc-plugin");
        embed.setAttribute("pluginspage", "http://www.videolan.org");
        // set compatibility flag
        embed.setAttribute("videocompat", "true");
        // handle multiple sources
        if (video.src) {
          embed.setAttribute("src", video.src);
        } else {
          let sources = video.getElementsByTagName("source");
          if (sources.length)
            embed.setAttribute("src", sources[0].src);
        }
        video.parentNode.replaceChild(embed, video);
        video.autoplay = false;
        video = null;
        vlcs.append(embed);
      }

      /* createDynamicMedia returns a new pseudo-<video>/<audio> object
       */
      function createDynamicMedia() {
        GM_log('createDynamicMedia called');

        var elem = _createElement.call(document, "embed");
        elem.setAttribute("type",        "application/x-vlc-plugin");
        elem.setAttribute("pluginspage", "http://www.videolan.org");
        // set compatibility flag
        elem.setAttribute("videocompat", "true");
        // dummy functions keep e.g. youtube happy whilst the actual plugin waits to be loaded
        elem.load = function () {};
        elem.play = function () {};
        elem.pause = function () {};
        elem.canPlayType = function (type) {
          return "maybe";
        };
        let _addEventListener = elem.addEventListener;
        elem._eventListenerMap = {
                "emptied": [],
                "loadedmetadata": [],
                "loadeddata": [],
                "canplay": [],
                "canplaythrough": [],
                "playing": [],
                "ended": [],
                "waiting": [],
                "ended": [],
                "durationchange": [],
                "timeupdate": [],
                "play": [],
                "pause": [],
                "ratechange": [],
                "volumechange": []
        };
        elem.addEventListener = function (type, func, capture) {
          if (type in elem._eventListenerMap) {
            elem._eventListenerMap[type].append(func);
          } else {
            _addEventListener.apply(elem, arguments);
          }
        };
        return elem;
      }

      // override createElement
      document.createElement = function VLC_createElement(tag) {
        try {
          switch (tag.toLowerCase()) {
            case "video":
            case "audio":
              return createDynamicMedia();
            default:
              return _createElement.call(document, tag);
          }
        } catch (e) {
          throw e;
        }
      };

      // these update live; no need to call each time
      var videos = document.getElementsByTagName("video");
      var audios = document.getElementsByTagName("audio");
      var embeds = document.getElementsByTagName("embed");

      // poor man's mutex
      var updating = false;

      // we listen to every DOM modification
      document.addEventListener("DOMSubtreeModified", function () {
        if (updating) return;
        updating = true;
        for (var i = 0; i < vlcs.length; ++i) {
          if (!!vlcs[i].play != vlcs[i]
        }
        try {
          // replace static video and audio
          while (videos.length)
            replaceStaticMedia(videos[0]);
          while (audios.length)
            replaceStaticMedia(audios[0]);
        } catch (e) {
          GM_log("static fail: " + e.message);
          // we failed.
        }
        updating = false;
      }, false);
    })();
  },
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([VLCVideo]);

