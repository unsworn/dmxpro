{
    "targets": [
        {
            "target_name": "dmxpro",
            "variables": {
                'node_version': '<!(node --version | sed -e "s/^v\([0-9]*\\.[0-9]*\).*$/\\1/")'
            },
            "sources": [         
                "src/DmxPro.cpp",
                "src/Serial.cpp"
            ],
            "conditions": [
                ['OS=="mac"',
                    {
                        'include_dirs': [],
                        "link_settings": {
                            'libraries': []
                        },
                    }
                ],
                ['OS=="linux"',
                    {
                        'link_settings': {
                            'libraries': []
                        }
                    }
                ]
            ],
            "target_conditions": [
                ['node_version=="0.8"', { 'defines': ['NODE_TARGET_VERSION=8'] } ]
            ]
        }
    ]
}
