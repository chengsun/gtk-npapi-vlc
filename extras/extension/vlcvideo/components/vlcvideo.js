let Cu = Components.utils;
let Ci = Components.interfaces;
let Cc = Components.classes;
let Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function GM_log(str) {
  dump(str);
  dump("\n");
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
      var _createElement = document.createElement;
      document.createElement = function VLC_createElement(tag) {
        try {
          switch (tag.toLowerCase()) {
            case "video":
            case "audio": {
              // hook into video dynamic creation
              GM_log('hook into video dynamic creation');
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
                GM_log("LISTENING TO " + type);
                /* TODO */
                _addEventListener.apply(elem, arguments);
              };
              elem.addEventListener("DOMAttrModified", function (e) {
                GM_log("MODIFIED " + e.relatedNode.name + " -> " + e.newValue);
              }, false);
              return elem;
            }
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
      document.addEventListener("DOMSubtreeModified", function () {
        //GM_log('hi');
        if (updating) return;
        updating = true;
        try {
          /*for (let i = 0; i < embeds.length; ++i) {
            let embed = embeds[i];
            if (embed.hasAttribute("vlcplaceholder")) {
              // finalise dynamically created video
              GM_log('finalising dynamically created video');
              embed.removeAttribute("vlcplaceholder");
              // handle multiple sources
              if (typeof embed.src !== "undefined") 
                embed.setAttribute("src", embed.src);
              // we can't use autoplay or loop, as they're reserved as "boolean" attributes and can only be set to themselves or nothing.
              if (typeof embed.autoplay !== "undefined")
                embed.setAttribute("autostart",       embed.autoplay.toString());
              if (typeof embed.loop !== "undefined")
                embed.setAttribute("autoloop",        embed.loop.toString());
              if (typeof embed.muted !== "undefined")
                embed.setAttribute("mute",            embed.muted.toString());
              if (typeof embed.controls !== "undefined") {
                GM_log("controls is " + embed.controls);
                embed.setAttribute("toolbar",         embed.controls.toString());
                }
              embed.setAttribute("allowfullscreen", "true");
              embed.setAttribute("bgcolor",         "#000000");
            }
          }*/
          function replaceStaticVideo(video) {
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
            // we can't use autoplay or loop, as they're reserved as "boolean" attributes and can only be set to themselves or nothing.
            /*embed.removeAttribute("autoplay");
            embed.removeAttribute("loop");
            embed.setAttribute("autostart",       video.autoplay.toString());
            embed.setAttribute("autoloop",        video.loop.toString());
            embed.setAttribute("mute",            video.muted.toString());
            embed.setAttribute("toolbar",         video.controls.toString());
            embed.setAttribute("allowfullscreen", "true");
            embed.setAttribute("bgcolor",         "#000000");*/

            video.parentNode.replaceChild(embed, video);
            video.autoplay = false;
            video = null;
          }
          // replace static video and audio
          while (videos.length)
            replaceStaticVideo(videos[0]);
          while (audios.length)
            replaceStaticVideo(audios[0]);
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

