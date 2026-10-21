# Canonical draw selection probe

The historical folder name is retained for this opt-in fixture. main.cpp now checks
only draw selection: weak membership, resource replacement, pick-ID invalidation,
clone independence and CPU serialization. Mode discriminators and production-resource
rejection assertions are removed with the old APIs. GPU picking still rejects a
missing device without clearing an existing selection. The agent did not compile
or execute this fixture.
