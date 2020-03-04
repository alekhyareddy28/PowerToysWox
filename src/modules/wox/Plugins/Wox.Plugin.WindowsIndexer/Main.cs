using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Wox.Plugin;
using Wox.Infrastructure.Storage;
using Wox.Plugin.WindowsIndexer;

namespace Windows.Plugin.Indexer
{
    class Main : IPlugin, ISavable, IPluginI18n
    {

        // This variable contains metadata about the Plugin
        private PluginInitContext _context;

        // This variable contains information about the context menus
        private Settings _settings;

        // Contains information about the plugin stored in json format
        private PluginJsonStorage<Settings> _storage;

        // To save the configurations of plugins
        public void Save()
        {
            _storage.Save();
        }

        // This function uses the Windows indexer and returns the list of results obtained
        public List<Result> Query(Query query)
        {
            var result = new Result
            {
                Title = "Windows Indexer Plugin",
                SubTitle = $"Query: {query.Search}",
                IcoPath = "Images\\WindowsIndexerImg.bmp"
            };
            return new List<Result> { result };
        }

        public void Init(PluginInitContext context)
        {
            // initialize the context of the plugin
            _context = context;
            _storage = new PluginJsonStorage<Settings>();
            _settings = _storage.Load();
        }

        // TODO: Localize the strings
        // Set the Plugin Title
        public string GetTranslatedPluginTitle()
        {
            return "Windows Indexer Plugin";
        }

        // TODO: Localize the string
        // Set the plugin Description
        public string GetTranslatedPluginDescription()
        {
            return "Returns files and folders";
        }
        

    }
}
